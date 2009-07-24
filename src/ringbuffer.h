/* $Id$ */

#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <jack/ringbuffer.h>

class RingBuffer {
private:
	jack_ringbuffer_t *m_buffer;

public:
	RingBuffer(size_t length)
	{
		m_buffer = jack_ringbuffer_create(length);
	}

	~RingBuffer()
	{
		jack_ringbuffer_free(m_buffer);
	}

	bool PushEvent(const jack_midi_event_t &event);
	bool PopEvent(jack_midi_event_t &event);
};

#endif /* RINGBUFFER_H */
