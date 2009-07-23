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
	RingBuffer *m_loop_buffer;

	bool           m_recording;
	bool           m_delay_record;
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

	const char *m_client_name;

	bool Connect();
	void Disconnect();
	bool Run();

	void ToggleRecording(int loop, int bpm, bool delay);
	void StartLoop(int loop, bool repeat, bool sync);
	void StopLoop(int loop);
	void EraseLoop(int loop);

	bool Connected() const
	{
		return m_connected;
	}

	bool Recording() const
	{
		return m_recording;
	}

	float LoopLength(int loop) const
	{
		return (float)m_loops[loop].Length() / m_sample_rate;
	}

	float LoopPosition(int loop) const
	{
		return (float)m_loops[loop].Position() / m_sample_rate;
	}

	LoopState GetLoopState(int loop) const
	{
		return m_loops[loop].State();
	}

	bool LoopLooping(int loop) const
	{
		return m_loops[loop].Looping();
	}

	void SetTempo(int loop, float tempo)
	{
		m_loops[loop].SetTempo(tempo);
	}

	float GetTempo(int loop) const
	{
		return m_loops[loop].Tempo();
	}

	void Save() const;
};

#endif /* JACK_H */
