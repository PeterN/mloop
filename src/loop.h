/* $Id$ */

#ifndef LOOP_H
#define LOOP_H

#include <stdint.h>
#include <list>
#include <jack/jack.h>
#include <jack/midiport.h>
#include "notecache.h"

enum LoopState {
	LS_IDLE,
	LS_SYNC,
	LS_PLAY,
	LS_STOPPING,
	LS_RECORDING,
};

typedef std::pair<jack_midi_event_t, jack_nframes_t> Event;
typedef std::list<Event> EventList;

class Loop {
private:
	jack_nframes_t m_length;   ///< Length of loop, in samples.
	jack_nframes_t m_position; ///< Current position of loop, in samples.
	LoopState      m_state;
	bool           m_loop;

	EventList           m_events;
	EventList::iterator m_iterator;

public:
	Loop();
	~Loop();

	void PlayFrame(void *port_buffer, jack_nframes_t frame);
	void AddEvent(jack_nframes_t position, jack_midi_event_t *event);

	void SetState(LoopState state)
	{
		m_state = state;
	}

	void SetLength(jack_nframes_t length);
	void Start(bool loop, bool sync);
	void Stop();
	void Empty();

	void StartFromNoteCache(NoteCache &cache);
	void EndFromNoteCache(NoteCache &cache);

	LoopState State() const
	{
		return m_state;
	}

	jack_nframes_t Length() const
	{
		return m_length;
	}

	jack_nframes_t Position() const
	{
		return m_position;
	}

	bool Looping() const
	{
		return m_loop;
	}
};

#endif /* LOOP_H */
