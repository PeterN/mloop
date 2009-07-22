/* $Id$ */

#include <unistd.h>
#include <limits.h>
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

	m_sample_rate = jack_get_sample_rate(m_client);

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
					fprintf(stderr, "Buffer full, dropping input!\n");
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
		m_loops[m_recording_loop].SetLength(m_recording_time);
	}

	return 0;
}

void Jack::ToggleRecording(int loop)
{
	if (m_recording) {
		m_recording = false;
		m_loops[m_recording_loop].SetLength(m_recording_time);
		m_loops[m_recording_loop].SetState(LS_IDLE);
		m_loops[m_recording_loop].EndFromNoteCache(m_notecache);
	} else {
		m_recording_loop = loop;
		m_loops[m_recording_loop].SetState(LS_RECORDING);
		m_loops[m_recording_loop].StartFromNoteCache(m_notecache);
		m_recording_time = 0;
		m_recording = true;
	}
}

void Jack::StartLoop(int loop, bool repeat)
{
	m_loops[loop].Start(repeat);
}

void Jack::StopLoop(int loop)
{
	m_loops[loop].Stop();
}

void Jack::EraseLoop(int loop)
{
	m_loops[loop].Empty();
}

bool Jack::Run()
{
	static jack_nframes_t recording_time;
	static jack_midi_event_t ev;
	static bool first = true;
	if (first) {
		ev.time = UINT_MAX;
		first = false;
	}

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
			m_loops[m_recording_loop].AddEvent(recording_time, &ev);
		}
		ev.time = UINT_MAX;
	}

	return false;
}

