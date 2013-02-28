#ifndef SCREEN_PICKER_H
#define SCREEN_PICKER_H

#include <Windows.h>

class ScreenPicker
{

public:
	
	ScreenPicker(){};
	~ScreenPicker(){};

	void Create(HINSTANCE inst, HWND parent);
	void start();


private:
	HINSTANCE _instance;
	HWND _parent_window;
	HWND _hMaskWin;

	void create_mask_window();
	static LRESULT CALLBACK proc_mask_window(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	BOOL handle_messages(UINT message, WPARAM wParam, LPARAM lParam);

};

#endif // SCREEN_PICKER_H