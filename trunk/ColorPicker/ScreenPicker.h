#ifndef SCREEN_PICKER_H
#define SCREEN_PICKER_H

#include <Windows.h>

class ScreenPicker
{

public:
	
	ScreenPicker(COLORREF color = 0);
	~ScreenPicker();

	void Create(HINSTANCE inst, HWND parent);
	bool IsCreated() {
		return (_mask_window != NULL);
	}
	
	void Color(COLORREF color);

	void Start();
	void End();
	
	bool IsShow() {
		return _is_shown;
	}

private:

	HINSTANCE _instance;
	HWND _parent_window;
	HWND _mask_window;
	HWND _info_window;

	bool _is_shown;

	HWND _zoom_area;

	HCURSOR _cursor;

	COLORREF _old_color;
	COLORREF _new_color;

	void CreateMaskWindow();
	static LRESULT CALLBACK MaskWindowWINPROC(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
	BOOL CALLBACK MaskWindowMessageHandle(UINT message, WPARAM wparam, LPARAM lparam);

	void CreateInfoWindow();
	static BOOL CALLBACK InfoWindowWINPROC(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
	BOOL CALLBACK InfoWindowMessageHandle(UINT message, WPARAM wparam, LPARAM lparam);

	void PrepareInfoWindow();

	void SampleColor(int x, int y);

};

#endif // SCREEN_PICKER_H