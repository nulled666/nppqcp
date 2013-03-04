#ifndef SCREEN_PICKER_H
#define SCREEN_PICKER_H

#include <Windows.h>

class ScreenPicker
{

public:
	
	ScreenPicker();
	~ScreenPicker() {};

	void Create(HINSTANCE inst, HWND parent);
	bool IsCreated() {
		return (_mask_window != NULL);
	}
	
	void Start();
	void End();
	

private:

	HINSTANCE _instance;
	HWND _parent_window;
	HWND _mask_window;

	HCURSOR _cursor;
	COLORREF _current_color;

	void CreateMaskWindow();
	static LRESULT CALLBACK MaskWindowWINPROC(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	BOOL MaskWindowMessageHandle(UINT message, WPARAM wParam, LPARAM lParam);

	void SampleColor(LPARAM lparam);

};

#endif // SCREEN_PICKER_H