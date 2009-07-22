/* $Id$ */

#ifndef JACK_H
#define JACK_H

#include <jack/jack.h>
#include "loop.h"
#include "ringbuffer.h"
#include "notecache.h"

#define NUM_LOOPS 10

class Jack {
private:
	bool           m_connected;
	jack_client_t *m_client;
	jack_port_t   *m_input;
	jack_port_t   *m_output;
	jack_port_t   *m_control;

	jack_nframes_t m_sample_rate;

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
	bool Run();

	void ToggleRecording(int loop);
	void StartLoop(int loop, bool repeat);
	void StopLoop(int loop);
	void EraseLoop(int loop);

	bool Recording() const
	{
		return m_recording;
	}

	float LoopLength(int loop)
	{
		return (float)m_loops[loop].Length() / m_sample_rate;
	}

	float LoopPosition(int loop)
	{
		return (float)m_loops[loop].Position() / m_sample_rate;
	}

	LoopState GetLoopState(int loop)
	{
		return m_loops[loop].State();
	}
};

#endif /* JACK_H */
