/* $Id$ */

#ifndef LOOP_H
#define LOOP_H

#include <stdint.h>
#include <list>
#include <jack/jack.h>
#include <jack/midiport.h>

enum LoopState {
	LS_IDLE,
	LS_PLAY_LOOP,
	LS_PLAY_ONCE,
	LS_STOPPING,
};

typedef std::pair<jack_midi_event_t, jack_nframes_t> Event;
typedef std::list<Event> EventList;

class Loop {
private:
	jack_nframes_t m_length;   ///< Length of loop, in samples.
	jack_nframes_t m_position; ///< Current position of loop, in samples.
	LoopState      m_state;

	EventList           m_events;
	EventList::iterator m_iterator;

public:
	void PlayFrame(void *port_buffer, jack_nframes_t frame);
	void AddEvent(jack_nframes_t position, jack_midi_event_t *event);

	void SetLength(jack_nframes_t length);
	void Start(bool loop);
	void Stop();
};

#endif /* LOOP_H */
