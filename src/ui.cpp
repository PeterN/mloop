/* $Id$ */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <curses.h>
#include <sys/select.h>
#include "jack.h"
#include "loop.h"
#include "ui.h"

int kbhit()
{
	struct timeval tv = { 0L, 0L };
	fd_set fds;
	FD_SET(0, &fds);
	return select(1, &fds, NULL, NULL, &tv);
}

int color_map[4];
char status[1024];

UI::UI()
{
	initscr();
	noecho();

	keypad(stdscr, true);
	nodelay(stdscr, true);
	raw();

	start_color();
	init_pair(1, COLOR_WHITE, COLOR_BLUE);
	init_pair(2, COLOR_BLACK, COLOR_CYAN);
	init_pair(3, COLOR_YELLOW, COLOR_RED);

	for (int i = 0; i < 4; i++) {
		color_map[i] = COLOR_PAIR(i);
	}

	snprintf(status, sizeof status, "...");

	m_loop = 0;
	m_bpm = 120;
	m_quantise = false;
	m_delay_record = false;
	m_sync_playback = false;
	m_edit_mode = EM_LOOPS;
	m_edit_timer = 0;
}

UI::~UI()
{
	endwin();
}

bool UI::Run(Jack &j)
{
	int y_offs = 0;

	bkgdset(color_map[1]);
	attrset(color_map[1]);
	mvaddstr(y_offs, 0, " mloop ");
	clrtoeol();
	y_offs++;

	bkgdset(color_map[0]);
	attrset(color_map[0]);
	mvaddstr(y_offs, 0, " [   ] bpm");
	clrtoeol();

	bkgdset(color_map[(m_edit_mode == EM_BPM) ? 2 : 0]);
	attrset(color_map[(m_edit_mode == EM_BPM) ? 2 : 0]);
	mvprintw(y_offs, 2, "%3d", m_bpm);
	y_offs++;

	bkgdset(color_map[0]);
	attrset(color_map[0]);
	mvprintw(y_offs, 0, " Quantisation: %s", m_quantise ? "on" : "off");
	clrtoeol();
	y_offs++;

	bkgdset(color_map[0]);
	attrset(color_map[0]);
	mvprintw(y_offs, 0, " Delay before record: %s", m_delay_record ? "on" : "off");
	clrtoeol();
	y_offs++;

	bkgdset(color_map[0]);
	attrset(color_map[0]);
	mvprintw(y_offs, 0, " Synchronise playback: %s", m_sync_playback ? "on" : "off");
	clrtoeol();
	y_offs++;

	bkgdset(color_map[1]);
	attrset(color_map[1]);
	mvaddstr(y_offs, 0, " Loops");
	clrtoeol();
	y_offs++;

	bkgdset(color_map[1]);
	attrset(color_map[1]);
	mvaddstr(y_offs + NUM_LOOPS, 0, status);
	clrtoeol();

	for (int i = 0; i < NUM_LOOPS; i++) {
		bkgdset(color_map[0]);
		attrset(color_map[0]);

		mvprintw(i + y_offs, 0, " [ ] %2d: Position: %0.2f beats (%0.2fs) Length: %0.2f beats (%0.2fs) Tempo: ", i, m_bpm * j.LoopPosition(i) / 60.0, j.LoopPosition(i), m_bpm * j.LoopLength(i) / 60.0, j.LoopLength(i));
		if (m_loop == i && m_edit_mode == EM_TEMPO) {
			bkgdset(color_map[2]);
			attrset(color_map[2]);
		}
		printw("%0.1f%%", j.GetTempo(i) * 100);
		bkgdset(color_map[0]);
		attrset(color_map[0]);
		clrtoeol();

		const char *c; int k = 3;
		switch (j.GetLoopState(i)) {
			case LS_IDLE: c = " "; k = 0; break;
			case LS_PLAY: c = j.LoopLooping(i) ? "»" : ">"; break;
			case LS_STOPPING: c = "·"; break;
			case LS_RECORDING: c = "R"; break;
			case LS_SYNC: c = "~"; break;
		}

		bkgdset(color_map[(m_loop == i && m_edit_mode == EM_LOOPS) ? 2 : k]);
		attrset(color_map[(m_loop == i && m_edit_mode == EM_LOOPS) ? 2 : k]);

		mvaddstr(i + y_offs, 2, c);
	}

	if (m_edit_timer > 0) m_edit_timer--;

	if (!kbhit()) {
		refresh();
		return false;
	}

	int c = getch();

	switch (c) {
		case UIKEY_QUIT:
			return true;

		case UIKEY_RECORD:
			j.ToggleRecording(m_loop, m_quantise ? m_bpm : 0, m_delay_record);
			if (j.Recording()) {
				snprintf(status, sizeof status, "Start recording loop %d", m_loop);
			} else {
				snprintf(status, sizeof status, "Finished recording loop");
			}
			break;

		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			if (m_edit_mode == EM_BPM) {
				if (m_edit_timer <= 0) {
					m_bpm = 0;
				}
				if (m_bpm < 100) {
					m_bpm *= 10;
					m_bpm += (c - '0');
				}
				m_edit_timer = EDIT_TIMER_RESET;
			} else if (m_edit_mode == EM_TEMPO) {
				if (m_edit_timer <= 0) {
					j.SetTempo(m_loop, 0.0);
				}
				float tempo = j.GetTempo(m_loop);
				if (tempo < 1000.0) {
					tempo *= 10;
					tempo += (c - '0') / 100.0;
					j.SetTempo(m_loop, tempo);
				}
				m_edit_timer = EDIT_TIMER_RESET;
			} else {
				m_loop = (c - '0');
				snprintf(status, sizeof status, "Selected loop %d", m_loop);
			}
			break;

		case UIKEY_PLAY_ONCE:
		case UIKEY_PLAY_LOOP:
			snprintf(status, sizeof status, "Starting loop %d (%s)", m_loop, c == UIKEY_PLAY_LOOP ? "loop" : "once");
			j.StartLoop(m_loop, c == UIKEY_PLAY_LOOP, m_sync_playback);
			break;

		case UIKEY_STOP:
			snprintf(status, sizeof status, "Stopping loop %d", m_loop);
			j.StopLoop(m_loop);
			break;

		case UIKEY_ERASE:
			snprintf(status, sizeof status, "Erasing loop %d", m_loop);
			j.EraseLoop(m_loop);
			break;

		case UIKEY_QUANTISE:
			m_quantise = !m_quantise;
			snprintf(status, sizeof status, "Set quantise %s", m_quantise ? "on" : "off");
			break;

		case UIKEY_DELAY:
			m_delay_record = !m_delay_record;
			break;

		case UIKEY_SYNC:
			m_sync_playback = !m_sync_playback;
			break;

		case UIKEY_BPM:
			m_edit_mode = EM_BPM;
			break;

		case UIKEY_TEMPO:
			m_edit_mode = EM_TEMPO;
			break;

		case '\r':
			m_edit_mode = EM_LOOPS;
			m_edit_timer = 0;
			break;

		case KEY_UP:
			if (m_edit_mode == EM_BPM) {
				m_bpm++;
				if (m_bpm >= 1000) m_bpm = 999;
			} else if (m_edit_mode == EM_TEMPO) {
				j.SetTempo(m_loop, j.GetTempo(m_loop) + 0.001);
			} else {
				m_loop--;
				if (m_loop == -1) m_loop = NUM_LOOPS - 1;
			}
			break;

		case KEY_DOWN:
			if (m_edit_mode == EM_BPM) {
				m_bpm--;
				if (m_bpm < 0) m_bpm = 0;
			} else if (m_edit_mode == EM_TEMPO) {
				j.SetTempo(m_loop, j.GetTempo(m_loop) - 0.001);
			} else {
				m_loop++;
				if (m_loop == NUM_LOOPS) m_loop = 0;
			}
			break;

		case KEY_BACKSPACE:
			if (m_edit_mode == EM_BPM) {
				m_bpm /= 10;
				m_edit_timer = EDIT_TIMER_RESET;
			} else if (m_edit_mode == EM_TEMPO) {
				j.SetTempo(m_loop, j.GetTempo(m_loop) / 10);
				m_edit_timer = EDIT_TIMER_RESET;
			}
			break;

		case UIKEY_SAVE:
			j.Save();
			break;
	}

	return false;
}
