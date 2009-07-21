/* $Id$ */

#include <stdio.h>
#include "loop.h"

void Loop::PlayFrame(void *port_buffer, jack_nframes_t frame)
{
	if (m_state == LS_IDLE) return;

	if (m_state == LS_STOPPING) {
		printf("Stopping, so send all notes off!\n");

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

		jack_midi_event_t &event = (*m_iterator).first;
		jack_nframes_t position = (*m_iterator).second;
		if (event.time + position > m_position) break;

		jack_midi_event_write(port_buffer, event.time, event.buffer, event.size);
	}

	m_position++;

	if (m_position == m_length) {
		if (m_state == LS_PLAY_ONCE) {
			m_state = LS_IDLE;
		}
		printf("Completed %u frames\n", m_position);
		m_position = 0;
		m_iterator = m_events.begin();
	}
}

void Loop::AddEvent(jack_nframes_t position, jack_midi_event_t *event)
{
	Event e;
	e.first = *event;
	e.second = position;
	m_events.push_back(e);
}

void Loop::SetLength(jack_nframes_t length)
{
	if (m_state != LS_IDLE) return;

	m_length = length;
}

void Loop::Start(bool loop)
{
	if (m_state != LS_IDLE) return;

	m_position = 0;
	m_iterator = m_events.begin();
	m_state = loop ? LS_PLAY_LOOP : LS_PLAY_ONCE;
}

void Loop::Stop()
{
	m_state = LS_STOPPING;
}


/*
void Loop::Reset()
{
	m_state = LS_IDLE;
	m_position = 0;
}
*/
