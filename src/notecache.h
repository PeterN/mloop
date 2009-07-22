/* $Id$ */

#ifndef NOTECACHE_H
#define NOTECACHE_H

#include <jack/midiport.h>

class NoteCache {
private:
	int8_t m_notes[16][128];

public:
	NoteCache();
	~NoteCache();

	void Reset();

	int8_t GetNote(int8_t channel, int8_t note) const
	{
		return m_notes[channel][note];
	}

	void HandleEvent(jack_midi_event_t &event);
};

#endif /* NOTECACHE_H */
