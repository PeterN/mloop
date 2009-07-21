/* $Id$ */

#ifndef JACK_H
#define JACK_H

#include <jack/jack.h>
#include "loop.h"
#include "ringbuffer.h"
#include "notecache.h"

#define NUM_LOOPS 9

class Jack {
private:
	bool           m_connected;
	jack_client_t *m_client;
	jack_port_t   *m_input;
	jack_port_t   *m_output;
	jack_port_t   *m_control;

	Loop        m_loops[NUM_LOOPS];
	RingBuffer *m_buffer;

	bool           m_recording;
	int            m_recording_loop;
	jack_nframes_t m_recording_time;

	NoteCache m_notecache;

	static void ShutdownCallbackHandler(void *arg)
	{
		((Jack *)arg)->ShutdownCallback();
	}
	static int ProcessCallbackHandler(jack_nframes_t nframes, void *arg)
	{
		return ((Jack *)arg)->ProcessCallback(nframes);
	}

	void ShutdownCallback();
	int ProcessCallback(jack_nframes_t nframes);

public:
	Jack();
	~Jack();

	bool Connect();
	void Disconnect();
	void Run();
};

#endif /* JACK_H */
