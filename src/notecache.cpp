/* $Id$ */

#include <string.h>
#include <jack/midiport.h>
#include "notecache.h"

NoteCache::NoteCache()
{
	memset(m_notes, 0, sizeof m_notes);
}

NoteCache::~NoteCache()
{
}

void NoteCache::Reset()
{
	memset(m_notes, 0, sizeof m_notes);
}

void NoteCache::HandleEvent(jack_midi_event_t &event)
{
	uint8_t *buffer = event.buffer;
	uint8_t cmd = buffer[0];

	/* Note on or off event */
	if ((cmd & 0xE0) == 0x80) {
		int8_t channel = cmd & 0x0F;

		for (uint i = 1; i < event.size; i += 2) {
			int8_t note = buffer[i] & 0x7F;
			int8_t velocity = buffer[i] & 0x7F;
			if ((cmd & 0xF0) == 0x80) velocity = 0;
			m_notes[channel][note] = velocity;
		}
	} else if ((cmd & 0xF0) == 0xB0 && buffer[1] == 0x7B) {
		/* All notes off */
		int8_t channel = cmd & 0x0F;
		memset(m_notes[channel], 0, sizeof m_notes[channel]);
	}
}

