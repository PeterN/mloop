/* $Id$ */

#include <stdio.h>
#include "loop.h"

Loop::Loop()
{
	m_length = 0;
	m_position = 0;
	m_tempo = 1.0;
	m_state = LS_IDLE;
}

Loop::~Loop()
{
}

void Loop::PlayFrame(void *port_buffer, jack_nframes_t frame)
{
	if (m_state == LS_IDLE || m_state == LS_RECORDING) return;

	if (m_state == LS_STOPPING) {
		uint8_t buffer[3];
		buffer[1] = 0x78;
		buffer[2] = 0;

		for (int i = 0; i < 16; i++) {
			buffer[0] = 0xB0 + i;
			jack_midi_event_write(port_buffer, frame, buffer, sizeof buffer);
		}

		m_state = LS_IDLE;
		return;
	}

	for (; m_iterator != m_events.end(); ++m_iterator) {
		jack_midi_event_t &event = *m_iterator;
		if (event.time > m_position) break;

		jack_midi_event_write(port_buffer, frame, event.buffer, event.size);
	}

	m_position += m_tempo;

	if (m_position >= m_length) {
		if (!m_loop) {
			m_state = LS_IDLE;
		}
		m_position = 0;
		m_iterator = m_events.begin();
	}
}

void Loop::AddEvent(jack_midi_event_t *event)
{
	m_events.push_back(*event);
}

void Loop::SetLength(jack_nframes_t length)
{
	if (m_state != LS_RECORDING) return;

	m_length = length;
}

void Loop::Start(bool loop, bool sync)
{
	if (m_state == LS_PLAY || m_state == LS_SYNC) {
		m_loop = loop;
	}

	if (m_state != LS_IDLE) return;

	if (m_length == 0) return;

	m_position = 0;
	m_iterator = m_events.begin();
	m_loop = loop;
	m_state = sync ? LS_PLAY : LS_PLAY;
}

void Loop::Stop()
{
	if (m_state == LS_PLAY) {
		m_state = LS_STOPPING;
	} else if (m_state == LS_SYNC) {
		m_state = LS_IDLE;
	}
}

void Loop::StartFromNoteCache(NoteCache &cache)
{
	for (uint8_t c = 0; c < 16; c++) {
		for (uint8_t n = 0; n < 128; n++) {
			int8_t v = cache.GetNote(c, n);

			if (v > 0) {
				uint8_t *buffer = (uint8_t *)malloc(3);
				buffer[0] = 0x90 + c;
				buffer[1] = n;
				buffer[2] = v;

				jack_midi_event_t event;
				event.time = 0;
				event.buffer = buffer;
				event.size = 3;

				AddEvent(&event);
			}
		}
	}
}

void Loop::EndFromNoteCache(NoteCache &cache)
{
	for (uint8_t c = 0; c < 16; c++) {
		for (uint8_t n = 0; n < 128; n++) {
			int8_t v = cache.GetNote(c, n);

			if (v > 0) {
				uint8_t *buffer = (uint8_t *)malloc(3);
				buffer[0] = 0x80 + c;
				buffer[1] = n;
				buffer[2] = 0;

				jack_midi_event_t event;
				event.time = m_length - 1;
				event.buffer = buffer;
				event.size = 3;

				AddEvent(&event);
			}
		}
	}
}

void Loop::Empty()
{
	if (m_state != LS_IDLE) return;

	m_length = 0;
	m_position = 0;

	for (m_iterator = m_events.begin(); m_iterator != m_events.end(); ++m_iterator) {
		jack_midi_event_t &event = *m_iterator;
		free(event.buffer);
	}

	m_events.clear();
}

void Loop::Save(FILE *f) const
{
	fprintf(f, "%u %f %f\n", m_length, m_position, m_tempo);
	fprintf(f, "%lu\n", m_events.size());

	EventList::const_iterator it;
	for (it = m_events.begin(); it != m_events.end(); ++it) {
		const jack_midi_event_t &ev = *it;
		fprintf(f, "%u %lu", ev.time, ev.size);
		for (uint i = 0; i < ev.size; i++) {
			fprintf(f, " %02X", ev.buffer[i]);
		}
		fprintf(f, "\n");
	}
}
