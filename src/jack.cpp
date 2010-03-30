/* $Id$ */

#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <jack/jack.h>
#include <jack/midiport.h>
#include "jack.h"

Jack::Jack()
{
	m_connected = false;
	m_recording = false;
	m_loop_buffer = new RingBuffer(2048);
	m_notecache.Reset();
	m_client_name = "mloop";
}

Jack::~Jack()
{
	Disconnect();
	delete m_loop_buffer;
}

bool Jack::Connect()
{
	if (m_connected) return true;

	jack_status_t status;
	m_client = jack_client_open(m_client_name, JackNoStartServer, &status);
	if (m_client == NULL) {
		if (status & JackServerFailed) {
			fprintf(stderr, "JACK server not running\n");
		} else {
			fprintf(stderr, "jack_client_open() failed, status = 0x%2.0x\n", status);
		}
		return false;
	}

	m_client_name = jack_get_client_name(m_client);
	m_sample_rate = jack_get_sample_rate(m_client);

	jack_on_shutdown(m_client, &ShutdownCallbackHandler, this);
	jack_set_process_callback(m_client, &ProcessCallbackHandler, this);

	m_input = jack_port_register(m_client, "input", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
	m_output = jack_port_register(m_client, "output", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);

	m_control = jack_port_register(m_client, "control", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput|JackPortIsTerminal, 0);

	m_connected = true;

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
				if (m_delay_record) m_recording_time = -ev.time;
				ev.time += m_recording_time;
				if (m_loop_buffer->PushEvent(ev)) {
					m_delay_record = false;
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

	if (m_recording && !m_delay_record) {
		m_recording_time += nframes;
		m_loops[m_recording_loop].SetLength(m_recording_time);
	}

	return 0;
}

void Jack::ToggleRecording(int loop, int bpm, bool delay)
{
	if (m_recording) {
		m_recording = false;

		if (bpm > 0) {
			jack_nframes_t chunk = 60 * m_sample_rate / bpm;
			m_recording_time -= m_recording_time % chunk;
		}

		m_loops[m_recording_loop].SetLength(m_recording_time);
		m_loops[m_recording_loop].SetState(LS_IDLE);
		m_loops[m_recording_loop].Finalise();
	} else {
		if (m_loops[loop].State() == LS_IDLE) {
			m_recording_loop = loop;
			m_loops[m_recording_loop].SetState(LS_RECORDING);
			m_loops[m_recording_loop].StartFromNoteCache(m_notecache);
			m_recording_time = 0;
			m_recording = true;
			m_delay_record = delay;
		}
	}
}

void Jack::StartLoop(int loop, bool repeat, bool sync)
{
	m_loops[loop].Start(repeat, sync);
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
	jack_midi_event_t ev;
	if (m_loop_buffer->PopEvent(ev)) {
		if (m_recording) {
			m_loops[m_recording_loop].AddEvent(&ev);
		} else {
			free(ev.buffer);
		}
	}

	return false;
}

void Jack::StopAll()
{
	for (int i = 0; i < NUM_LOOPS; i++) {
		StopLoop(i);
	}
}

void Jack::EraseAll()
{
	for (int i = 0; i < NUM_LOOPS; i++) {
		EraseLoop(i);
	}
}

void Jack::Save() const
{
	FILE *f = fopen("test.txt", "w");
	fprintf(f, "sample_rate:%d bpm:%u\n", m_sample_rate);
	for (int i = 0; i < NUM_LOOPS; i++) {
		fprintf(f, "loop:%d\n", i);
		m_loops[i].Save(f);
	}
	fclose(f);
}

void Jack::Load()
{
	StopAll();
	EraseAll();

	int ret;
	FILE *f = fopen("test.txt", "r");

	int sample_rate;
	ret = fscanf(f, "sample_rate:%d\n", &sample_rate);
	if (ret != 1) return;

	while (!feof(f)) {
		int i;

		ret = fscanf(f, "loop:%d\n", &i);
		if (ret != 1) return;

		m_loops[i].Load(f, sample_rate, m_sample_rate);
	}
}
