/* $Id$ */

#ifndef UI_H
#define UI_H

#define EDIT_TIMER_RESET 200

enum EditMode {
	EM_LOOPS,
	EM_BPM,
	EM_TEMPO,
};

class UI {
private:
	int m_loop;
	int m_bpm;
	bool m_quantise;
	bool m_delay_record;
	bool m_sync_playback;

	EditMode m_edit_mode;
	int m_edit_timer;

public:
	UI();
	~UI();

	bool Run(Jack &j);
};

#define UIKEY_QUIT 3
#define UIKEY_RECORD 'r'
#define UIKEY_PLAY_ONCE 'z'
#define UIKEY_PLAY_LOOP 'x'
#define UIKEY_STOP 'c'
#define UIKEY_STOP_ALL 'C'
#define UIKEY_ERASE 'e'
#define UIKEY_QUANTISE 'q'
#define UIKEY_DELAY 'd'
#define UIKEY_SYNC 's'
#define UIKEY_BPM 'b'
#define UIKEY_TEMPO 't'
#define UIKEY_SAVE 'S'
#define UIKEY_LOAD 'L'
#define UIKEY_RECONNECT 18
#define UIKEY_DISCONNECT 4

#endif /* UI_H */
