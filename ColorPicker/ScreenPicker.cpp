
#include <iostream>
#include <Windowsx.h>

#include "ScreenPicker.h"
#include "ColorPicker.res.h"

ScreenPicker::ScreenPicker(){

	_instance = NULL;
	_parent_window = NULL;
	_mask_window = NULL;

	_current_color = 0;

}

void ScreenPicker::Create(HINSTANCE inst, HWND parent){
	
	if (IsCreated()) {
		throw std::runtime_error("ScreenPicker::Create() : duplicate creation");
	}

	if (!inst) {
		throw std::exception("ScreenPicker : instance handle required");
	}

	_instance = inst;
	_parent_window = parent;
	
	CreateMaskWindow();
	CreateInfoWindow();

}


void ScreenPicker::CreateMaskWindow(){
		
	static wchar_t szWindowClass[] = L"nplus_screen_picker";
	
	_cursor = ::LoadCursor(_instance, MAKEINTRESOURCE(IDI_CURSOR));

	WNDCLASSEX wc    = {0};
	wc.cbSize        = sizeof(wc);
	wc.lpfnWndProc   = MaskWindowWINPROC;
	wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = _instance;
	wc.hCursor       = _cursor;
	wc.lpszClassName = szWindowClass;

	if (!RegisterClassEx(&wc)) {
		throw std::runtime_error("ScreenPicker: RegisterClassEx() failed");
	}

	_mask_window = CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT, szWindowClass, szWindowClass, WS_POPUP, 0, 0, 0, 0, NULL, NULL, _instance, this);

	if (!_mask_window) {
		throw std::runtime_error("ScreenPicker: CreateWindowEx() function returns null");
	}

}

LRESULT CALLBACK ScreenPicker::MaskWindowWINPROC(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {

	switch (message) {
		case WM_NCCREATE:
		{
			CREATESTRUCT*  cs = (CREATESTRUCT*)lparam;
			ScreenPicker* pScreenPicker = (ScreenPicker*)(cs->lpCreateParams);
			pScreenPicker->_mask_window = hwnd;
			::SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pScreenPicker);
			return pScreenPicker->MaskWindowMessageHandle(message, wparam, lparam);
		}
		default:
		{
			ScreenPicker* pScreenPicker = reinterpret_cast<ScreenPicker*>(::GetWindowLongPtr(hwnd, GWL_USERDATA));
			if (!pScreenPicker)
				return FALSE;
			return pScreenPicker->MaskWindowMessageHandle(message, wparam, lparam);
		}
	}
}

BOOL ScreenPicker::MaskWindowMessageHandle(UINT message, WPARAM wparam, LPARAM lparam) {

	switch (message) {
		case WM_MOUSEMOVE:
		{
			::SetCursor(_cursor);
			SampleColor(lparam);
			return TRUE;
		}
		case WM_ACTIVATE:
		{
			return 0;
		}
		case WM_LBUTTONUP:
		{
			End();
			::SendMessage(_parent_window, WM_SCREEN_PICK_COLOR, 0, (LPARAM)_current_color);
			return TRUE;
		}
		case WM_RBUTTONUP:
		{
			End();
			::SendMessage(_parent_window, WM_SCREEN_PICK_CANCEL, 0, 0);
			return TRUE;
		}
		default:
		{
			return TRUE;
		}
	}
	
}


void ScreenPicker::Start(){

	HMONITOR monitor = ::MonitorFromWindow(_parent_window, MONITOR_DEFAULTTONEAREST);
	MONITORINFO mi;
	mi.cbSize = sizeof(MONITORINFO);
	::GetMonitorInfo(monitor, (LPMONITORINFO)&mi);
	int width = mi.rcMonitor.right - mi.rcMonitor.left;
	int height = mi.rcMonitor.bottom - mi.rcMonitor.top;

	::SetWindowPos(_mask_window, HWND_TOPMOST, mi.rcMonitor.left, mi.rcMonitor.top, width, height, SWP_SHOWWINDOW);

}

void ScreenPicker::End(){

	::SetWindowPos(_mask_window, HWND_TOPMOST, 0, 0, 0, 0, SWP_HIDEWINDOW);
	::SetWindowPos(_info_window, HWND_TOP, 0, 0, 0, 0, SWP_HIDEWINDOW);

}

void ScreenPicker::SampleColor(LPARAM lparam){
	
	int x = GET_X_LPARAM(lparam);
	int y = GET_Y_LPARAM(lparam);

	HDC hdc = ::GetDC(HWND_DESKTOP);
	_current_color = ::GetPixel(hdc, x, y);
	::ReleaseDC(HWND_DESKTOP, hdc);

	::SendMessage(_parent_window, WM_SCREEN_HOVER_COLOR, 0, (LPARAM)_current_color);

	::SetWindowPos(_info_window, HWND_TOP, x+10, y-130, 120, 120, SWP_SHOWWINDOW);

}


// the information window
void ScreenPicker::CreateInfoWindow(){
		
	
	_info_window = ::CreateDialogParam(_instance, MAKEINTRESOURCE(IDD_SCREEN_PICKER_POPUP), NULL, (DLGPROC)InfoWindowWINPROC, (LPARAM)this);

	
}

BOOL CALLBACK ScreenPicker::InfoWindowWINPROC(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {

	switch (message) {
		case WM_INITDIALOG:
		{
			ScreenPicker *pScreenPicker = (ScreenPicker*)(lparam);
			pScreenPicker->_info_window = hwnd;
			::SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)lparam);
			pScreenPicker->InfoWindowMessageHandle(message, wparam, lparam);
			return TRUE;
		}
		default:
		{
			ScreenPicker *pScreenPicker = reinterpret_cast<ScreenPicker*>(::GetWindowLongPtr(hwnd, GWL_USERDATA));
			if (!pScreenPicker)
				return FALSE;
			return pScreenPicker->InfoWindowMessageHandle(message, wparam, lparam);
		}
	}

}


BOOL ScreenPicker::InfoWindowMessageHandle(UINT message, WPARAM wparam, LPARAM lparam) {

	switch (message) {
		case WM_INITDIALOG:
		{
			PrepareInfoWindow();
			return TRUE;
		}
		default:
		{
			return FALSE;
		}
	}
	
}


void ScreenPicker::PrepareInfoWindow(){

	::SetWindowPos(_info_window, HWND_TOP, 0, 0, 120, 120, SWP_SHOWWINDOW);

}