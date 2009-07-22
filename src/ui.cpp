/* $Id$ */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <curses.h>
#include <sys/select.h>
#include <termios.h>
#include "jack.h"
#include "loop.h"
#include "ui.h"

struct termios orig_termios;

void reset_terminal_mode()
{
	tcsetattr(0, TCSANOW, &orig_termios);
}

void set_conio_terminal_mode()
{
	struct termios new_termios;

	/* take two copies - one for now, one for later */
	tcgetattr(0, &orig_termios);
#if 0
	memcpy(&new_termios, &orig_termios, sizeof(new_termios));

	/* register cleanup handler, and set the new terminal mode */
	atexit(reset_terminal_mode);
	cfmakeraw(&new_termios);
	tcsetattr(0, TCSANOW, &new_termios);
#endif
}

int kbhit()
{
	struct timeval tv = { 0L, 0L };
	fd_set fds;
	FD_SET(0, &fds);
	return select(1, &fds, NULL, NULL, &tv);
}

/*
int getch()
{
	int r;
	unsigned char c;
	if ((r = read(0, &c, sizeof(c))) < 0) {
		return r;
	} else {
		return c;
	}
}
*/

int bpm = 120;
int color_map[4];
char status[1024];

bool UI::Run(Jack &j)
{
	char buf[1024];
	static bool first = true;
	if (first) {
		set_conio_terminal_mode();
		first = false;
		m_loop = 0;

		initscr();
		noecho();
		nonl();
		intrflush(stdscr, false);
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
	}

	snprintf(buf, sizeof buf, " mloop -- %d bpm", bpm);
	bkgdset(color_map[1]);
	attrset(color_map[1]);
	mvaddstr(0, 0, buf);
	clrtoeol();

	mvaddstr(11, 0, status);
	clrtoeol();

	for (int i = 0; i < NUM_LOOPS; i++) {
		bkgdset(color_map[0]);
		attrset(color_map[0]);

		snprintf(buf, sizeof buf, " [ ] %2d: Position: %0.2f beats (%0.2fs) Length: %0.2f beats (%0.2fs)", i, bpm * j.LoopPosition(i) / 60.0, j.LoopPosition(i), bpm * j.LoopLength(i) / 60.0, j.LoopLength(i));
		mvaddstr(i + 1, 0, buf);
		clrtoeol();

		const char *c; int k = 3;
		switch (j.GetLoopState(i)) {
			case LS_IDLE: c = " "; k = 0; break;
			case LS_PLAY_ONCE: c = ">"; break;
			case LS_PLAY_LOOP: c = "»"; break;
			case LS_STOPPING: c = "·"; break;
			case LS_RECORDING: c = "R"; break;
		}

		bkgdset(color_map[m_loop == i ? 2 : k]);
		attrset(color_map[m_loop == i ? 2 : k]);

		mvaddstr(i + 1, 2, c);
	}

	if (!kbhit()) {
		refresh();
		return false;
	}

	int c = getch();

	switch (c) {
		case 3:
		case 'q': {
			endwin();
			reset_terminal_mode();
			return true;
		}

		case 'r':
			j.ToggleRecording(m_loop);
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
			m_loop = (c - '0');
			snprintf(status, sizeof status, "Selected loop %d", m_loop);
			break;

		case 'z':
		case 'x':
			snprintf(status, sizeof status, "Starting loop %d (%s)", m_loop, c == 'x' ? "loop" : "once");
			j.StartLoop(m_loop, c == 'x');
			break;

		case 'c':
			snprintf(status, sizeof status, "Stopping loop %d", m_loop);
			j.StopLoop(m_loop);
			break;

		case 'e':
			snprintf(status, sizeof status, "Erasing loop %d", m_loop);
			j.EraseLoop(m_loop);
			break;

		case KEY_UP:
			m_loop--;
			if (m_loop == -1) m_loop = NUM_LOOPS - 1;
			break;

		case KEY_DOWN:
			m_loop++;
			if (m_loop == NUM_LOOPS) m_loop = 0;
			break;
	}

	return false;
}
