/* $Id$ */

#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <sys/select.h>
#include <termios.h>
#include <jack/jack.h>
#include <jack/midiport.h>
#include "jack.h"

Jack::Jack()
{
	m_connected = false;
	m_recording = false;
	m_buffer = new RingBuffer(2048);
	m_notecache.Reset();
}

Jack::~Jack()
{
	Disconnect();
	delete m_buffer;
}

bool Jack::Connect()
{
	if (m_connected) return true;

	jack_status_t status;
	m_client = jack_client_open("mloop", JackNoStartServer, &status);
	if (m_client == NULL) {
		if (status & JackServerFailed) {
			fprintf(stderr, "JACK server not running\n");
		} else {
			fprintf(stderr, "jack_client_open() failed, status = 0x%2.0x\n", status);
		}
		return false;
	}

	m_connected = true;

	jack_on_shutdown(m_client, &ShutdownCallbackHandler, this);
	jack_set_process_callback(m_client, &ProcessCallbackHandler, this);

	m_input = jack_port_register(m_client, "input", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
	m_output = jack_port_register(m_client, "output", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);

	m_control = jack_port_register(m_client, "control", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput|JackPortIsTerminal, 0);

	jack_activate(m_client);

	return true;
}

void Jack::Disconnect()
{
	if (!m_connected) return;

	m_connected = false;

	jack_deactivate(m_client);
	jack_client_close(m_client);
}

void Jack::ShutdownCallback()
{
	m_connected = false;
	printf("shutdowncallback\n");
}

int Jack::ProcessCallback(jack_nframes_t nframes)
{
	void *input   = jack_port_get_buffer(m_input, nframes);
	void *output  = jack_port_get_buffer(m_output, nframes);

	void *control = jack_port_get_buffer(m_control, nframes);

	jack_midi_clear_buffer(output);

	/* Copy from input to output */
	jack_nframes_t event_count = jack_midi_get_event_count(input);
	jack_nframes_t event_index = 0;

	jack_midi_event_t ev;
	if (event_index < event_count) {
		jack_midi_event_get(&ev, input, event_index++);
	} else {
		ev.time = UINT_MAX;
	}

	for (jack_nframes_t frame = 0; frame < nframes; frame++) {
		while (ev.time == frame) {
			jack_midi_event_write(output, ev.time, ev.buffer, ev.size);

			m_notecache.HandleEvent(ev);

			if (m_recording) {
				/* Don't add the event to the buffer if it will become full.
				 * This includes the case where the event would actually fit,
				 * but would cause the buffer to be full. This prevents the
				 * need for extra logic to determine if the buffer is full
				 * or empty.
				 */
				if (m_buffer->Free() > sizeof ev.time + sizeof ev.size + ev.size) {
					m_buffer->Write((uint8_t *)&m_recording_time, sizeof m_recording_time);
					m_buffer->Write((uint8_t *)&ev.time, sizeof ev.time);
					m_buffer->Write((uint8_t *)&ev.size, sizeof ev.size);
					m_buffer->Write((uint8_t *)ev.buffer, ev.size);
				} else {
					printf("Buffer full, dropping input!\n");
				}
			}

			if (event_index < event_count) {
				jack_midi_event_get(&ev, input, event_index++);
			} else {
				ev.time = UINT_MAX;
			}
		}

		for (int i = 0; i < NUM_LOOPS; i++) {
			m_loops[i].PlayFrame(output, frame);
		}
	}

	if (m_recording) {
		m_recording_time += nframes;
	}

	return 0;
}

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

void Jack::Run()
{
	set_conio_terminal_mode();

	jack_nframes_t recording_time;
	jack_midi_event_t ev;
	ev.time = UINT_MAX;

	m_recording = false;

	for (;;) {
		usleep(10000);
		if (ev.time == UINT_MAX) {
			if (m_buffer->Size() >= sizeof recording_time + sizeof ev.time + sizeof ev.size) {
				m_buffer->Read((uint8_t *)&recording_time, sizeof recording_time);
				m_buffer->Read((uint8_t *)&ev.time, sizeof ev.time);
				m_buffer->Read((uint8_t *)&ev.size, sizeof ev.size);
			}
		} else {
			if (m_buffer->Size() >= ev.size) {
				ev.buffer = (jack_midi_data_t *)malloc(ev.size);
				m_buffer->Read((uint8_t *)ev.buffer, ev.size);
			}

			if (m_recording) {
				printf("Recording event for loop %d\n", m_recording_loop);
				m_loops[m_recording_loop].AddEvent(recording_time, &ev);
			}
			ev.time = UINT_MAX;
		}

		if (kbhit()) {
			char c = getch();

			switch (c) {
				case 3:
				case 'q': return;

				case 'r':
					if (m_recording) {
						m_recording = false;
						m_loops[m_recording_loop].SetLength(m_recording_time);
						m_loops[m_recording_loop].EndFromNoteCache(m_notecache);
						printf("Finished recording loop %d\n", m_recording_loop);
					} else {
						m_loops[m_recording_loop].StartFromNoteCache(m_notecache);
						m_recording_time = 0;
						m_recording = true;
						printf("Started recording loop %d\n", m_recording_loop);
					}
					break;

				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
					if (!m_recording) {
						m_recording_loop = c - '1';
						printf("Selected recording loop %d\n", m_recording_loop);
					}
					break;

				case 'z':
				case 'x':
					printf("Starting loop %d (%s)\n", m_recording_loop, c == 'x' ? "loop" : "once");
					m_loops[m_recording_loop].Start(c == 'x');
					break;

				case 'c':
					printf("Stopping loop %d\n", m_recording_loop);
					m_loops[m_recording_loop].Stop();
					break;

				case 'e':
					printf("Erasing loop %d\n", m_recording_loop);
					m_loops[m_recording_loop].Empty();
					break;
			}
		}
	}
}
