
#include <iostream>
#include "ScreenPicker.h"

void ScreenPicker::Create(HINSTANCE inst, HWND parent){
	
	if(!inst){
		throw std::exception("ScreenPicker : instance inst required");
	}

	if(!parent){
		throw std::exception("ScreenPicker : parent hwnd required");
	}

	_instance = inst;
	_parent_window = parent;

	create_mask_window();

}


void ScreenPicker::create_mask_window(){
		
	static wchar_t szWindowClass[] = L"np_screen_picker";
	
	WNDCLASSEX wc    = {0};
	wc.cbSize        = sizeof(wc);
	wc.lpfnWndProc   = proc_mask_window;
	wc.cbClsExtra     = 0;
    wc.cbWndExtra     = 0;
    wc.hInstance      = _instance;
	wc.lpszClassName = szWindowClass;

	if (!RegisterClassEx(&wc)) {
		throw std::runtime_error("ScreenPicker : RegisterClassEx() failed");
	}

	_hMaskWin = CreateWindowEx(0, szWindowClass, szWindowClass, WS_POPUP,	0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL);

	if (!_hMaskWin) {
		throw std::runtime_error("ScreenPicker : CreateWindowEx() function returns null");
	}

}


// message proccessor
LRESULT CALLBACK ScreenPicker::proc_mask_window(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG :
		{
			ScreenPicker *pScreenPicker = (ScreenPicker *)(lParam);
			pScreenPicker->_hMaskWin = hwnd;
			::SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)lParam);
			pScreenPicker->handle_messages(message, wParam, lParam);
			return TRUE;
		}

		default :
		{
			ScreenPicker *pScreenPicker = reinterpret_cast<ScreenPicker *>(::GetWindowLongPtr(hwnd, GWL_USERDATA));
			if (!pScreenPicker)
				return FALSE;
			return pScreenPicker->handle_messages(message, wParam, lParam);
		}
	}

	return TRUE;
}


BOOL ScreenPicker::handle_messages(UINT message, WPARAM wParam, LPARAM lParam){
	return TRUE;
}