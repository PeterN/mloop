/* $Id$ */

#include <stdio.h>
#include <string.h>
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
		/* Iterate the remainder of the loop but only send note off events */
		for (; m_iterator != m_events.end(); ++m_iterator) {
			jack_midi_event_t &event = *m_iterator;
			if ((event.buffer[0] & 0xF0) == 0x80) {
				jack_midi_event_write(port_buffer, frame, event.buffer, event.size);
			}
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

void Loop::Finalise()
{
	NoteCache nc;

	/* Remove all events after the end of the loop. */
	EventList::iterator it;
	for (it = m_events.begin(); it != m_events.end();) {
		jack_midi_event_t &ev = *it;
		if (ev.time < m_length) {
			nc.HandleEvent(ev);
			++it;
		} else {
			it = m_events.erase(it);
		}
	}

	/* Now make sure no notes are left on. */
	EndFromNoteCache(nc);
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
	fprintf(f, "length:%u position:%f tempo:%f events:%lu\n", m_length, m_position, m_tempo, (unsigned long)m_events.size());

	EventList::const_iterator it;
	for (it = m_events.begin(); it != m_events.end(); ++it) {
		const jack_midi_event_t &ev = *it;
		fprintf(f, "time:%u size:%lu", ev.time, (unsigned long)ev.size);
		for (uint i = 0; i < ev.size; i++) {
			fprintf(f, " %02X", ev.buffer[i]);
		}
		fprintf(f, "\n");
	}
}

void Loop::Load(FILE *f, int file_sample_rate, int jack_sample_rate)
{
	int ret;
	unsigned long events;

	ret = fscanf(f, "length:%u position:%f tempo:%f events:%lu\n", &m_length, &m_position, &m_tempo, &events);
	if (ret != 4) return;

	while (events--) {
		char *data;
		unsigned long size;
		jack_midi_event_t ev;

		ret = fscanf(f, "time:%u size:%lu %a[0-9A-F ]\n", &ev.time, &size, &data);
		if (ret != 3) return;
		//ev.time = ev.time * jack_sample_rate / file_sample_rate;
		ev.size = (size_t)size;
		ev.buffer = (unsigned char *)malloc(ev.size);

		/* Yup, no bounds checking... */
		unsigned char *p = ev.buffer;
		char *q = data;
		while (size--) {
			*p++ = strtol(q, NULL, 16);
			q += 3;
		}
		free(data);

		AddEvent(&ev);
	}
}
