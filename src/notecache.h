/* $Id$ */

#ifndef NOTECACHE_H
#define NOTECACHE_H

#include <stdio.h>
#include <string.h>

class NoteCache {
private:
	int8_t m_notes[16][128];

public:
	NoteCache()
	{
		memset(m_notes, 0, sizeof m_notes);
	}

	~NoteCache()
	{
	}

	void Reset()
	{
		memset(m_notes, 0, sizeof m_notes);
	}

	int8_t GetNote(int8_t channel, int8_t note) const
	{
		return m_notes[channel][note];
	}

	void HandleEvent(jack_midi_event_t &event)
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
};

#endif /* NOTECACHE_H */
