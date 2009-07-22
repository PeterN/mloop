/* $Id$ */

#ifndef UI_H
#define UI_H

#define EDIT_TIMER_RESET 200

enum EditMode {
	EM_LOOPS,
	EM_BPM,
};

class UI {
private:
	int m_loop;
	int m_bpm;
	bool m_quantise;

	EditMode m_edit_mode;
	int m_edit_timer;

public:
	UI();
	~UI();

	bool Run(Jack &j);
};

#endif /* UI_H */
