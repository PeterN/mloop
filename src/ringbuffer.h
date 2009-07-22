/* $Id$ */

#ifndef RINGBUFFER_H
#define RINGBUFFER_H

class RingBuffer {
private:
	uint8_t *m_buffer;
	uint8_t *m_end;
	uint8_t *m_write;
	uint8_t *m_read;

public:
	RingBuffer(size_t length)
	{
		m_buffer = (uint8_t *)malloc(length);
		m_end = m_buffer + length;
		m_write = m_buffer;
		m_read = m_buffer;
	}

	~RingBuffer()
	{
		free(m_buffer);
	}

	void Write(const uint8_t *buffer, size_t length)
	{
		while (length--) {
			if (m_write == m_end) m_write = m_buffer;
			*m_write++ = *buffer++;
		}
	}

	void Read(uint8_t *buffer, size_t length)
	{
		while (length--) {
			if (m_read == m_end) m_read = m_buffer;
			*buffer++ = *m_read++;
		}
	}

	uint8_t *Peek(uint8_t *buffer, size_t length, uint8_t *peek)
	{
		while (length--) {
			if (peek == m_end) peek = m_buffer;
			*buffer++ = *peek++;
		}
		return peek;
	}

	size_t Size() const
	{
		if (m_write >= m_read) return m_write - m_read;
		return (m_end - m_read) + (m_write - m_buffer);
	}

	size_t Free() const
	{
		return (m_end - m_buffer) - Size();
	}

	void Reset()
	{
		m_write = m_buffer;
		m_read = m_buffer;
	}

	bool PushEvent(jack_midi_event_t &event);
	bool PopEvent(jack_midi_event_t &event);
};

#endif /* RINGBUFFER_H */
