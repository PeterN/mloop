/* $Id$ */

#include <stdint.h>
#include <jack/midiport.h>
#include "ringbuffer.h"

bool RingBuffer::PushEvent(jack_midi_event_t &ev)
{
	if (Free() <= sizeof ev.time + sizeof ev.size + ev.size) return false;

	Write((uint8_t *)&ev.time, sizeof ev.time);
	Write((uint8_t *)&ev.size, sizeof ev.size);
	Write((uint8_t* )ev.buffer, ev.size);

	return true;
}

bool RingBuffer::PopEvent(jack_midi_event_t &ev)
{
	if (Size() >= sizeof ev.time + sizeof ev.size) {
		uint8_t *peek;
		peek = Peek((uint8_t *)&ev.time, sizeof ev.time, m_read);
		peek = Peek((uint8_t *)&ev.size, sizeof ev.size, peek);
		if (Size() >= sizeof ev.time + sizeof ev.size + ev.size) {
			ev.buffer = (jack_midi_data_t *)malloc(ev.size);
			m_read = peek;
			Read((uint8_t *)ev.buffer, ev.size);
			return true;
		}
	}
	return false;
}
