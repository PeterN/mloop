/* $Id$ */

#include <stdint.h>
#include <jack/midiport.h>
#include "ringbuffer.h"

bool RingBuffer::PushEvent(const jack_midi_event_t &ev)
{
	size_t f = jack_ringbuffer_write_space(m_buffer);
	if (f <= sizeof ev.time + sizeof ev.size + ev.size) return false;

	jack_ringbuffer_write(m_buffer, (const char *)&ev.size, sizeof ev.size);
	jack_ringbuffer_write(m_buffer, (const char *)&ev.time, sizeof ev.time);
	jack_ringbuffer_write(m_buffer, (const char *)ev.buffer, ev.size);

	return true;
}

bool RingBuffer::PopEvent(jack_midi_event_t &ev)
{
	size_t f = jack_ringbuffer_read_space(m_buffer);
	if (f >= sizeof ev.time + sizeof ev.size) {
		jack_ringbuffer_peek(m_buffer, (char *)&ev.size, sizeof ev.size);
		if (f >= sizeof ev.time + sizeof ev.size + ev.size) {
			ev.buffer = (jack_midi_data_t *)malloc(ev.size);
			jack_ringbuffer_read(m_buffer, (char *)&ev.size, sizeof ev.size);
			jack_ringbuffer_read(m_buffer, (char *)&ev.time, sizeof ev.time);
			jack_ringbuffer_read(m_buffer, (char *)ev.buffer, ev.size);
			return true;
		}
	}
	return false;
}
