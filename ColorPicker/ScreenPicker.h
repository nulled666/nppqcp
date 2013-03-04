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
	HWND _info_window;

	HWND _zoom_area;
	HWND _color_hex;
	HWND _color_rgb;

	HCURSOR _cursor;
	COLORREF _current_color;

	void CreateMaskWindow();
	static LRESULT CALLBACK MaskWindowWINPROC(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
	BOOL CALLBACK MaskWindowMessageHandle(UINT message, WPARAM wparam, LPARAM lparam);

	void CreateInfoWindow();
	static BOOL CALLBACK InfoWindowWINPROC(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
	BOOL CALLBACK InfoWindowMessageHandle(UINT message, WPARAM wparam, LPARAM lparam);

	void PrepareInfoWindow();

	void SampleColor(LPARAM lparam);

};

#endif // SCREEN_PICKER_H