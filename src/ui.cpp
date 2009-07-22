/* $Id$ */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/select.h>
#include <termios.h>
#include "jack.h"
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
	memcpy(&new_termios, &orig_termios, sizeof(new_termios));

	/* register cleanup handler, and set the new terminal mode */
	atexit(reset_terminal_mode);
	cfmakeraw(&new_termios);
	tcsetattr(0, TCSANOW, &new_termios);
}

int kbhit()
{
	struct timeval tv = { 0L, 0L };
	fd_set fds;
	FD_SET(0, &fds);
	return select(1, &fds, NULL, NULL, &tv);
}

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

bool UI::Run(Jack &j)
{
	static bool first = true;
	if (first) {
		set_conio_terminal_mode();
		first = false;
		m_loop = 0;
	}

	if (!kbhit()) return false;

	char c = getch();

	switch (c) {
		case 3:
		case 'q': return true;

		case 'r':
			j.ToggleRecording(m_loop);
			printf("%s recording loop\n", j.Recording() ? "Started" : "Finished");
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
			printf("Selected loop %d\n", m_loop);
			break;

		case 'z':
		case 'x':
			printf("Starting loop %d (%s)\n", m_loop, c == 'x' ? "loop" : "once");
			j.StartLoop(m_loop, c == 'x');
			break;

		case 'c':
			printf("Stopping loop %d\n", m_loop);
			j.StopLoop(m_loop);
			break;

		case 'e':
			printf("Erasing loop %d\n", m_loop);
			j.EraseLoop(m_loop);
			break;
	}

	return false;
}
