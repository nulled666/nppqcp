
#include <iostream>
#include <Windowsx.h>

#include "ScreenPicker.h"
#include "ColorPicker.res.h"

#define MASK_WIN_CLASS L"nplus_screen_picker"
#define INFO_WINDOW_WIDTH 180
#define INFO_WINDOW_HEIGHT 100
#define SWATCH_BG_COLOR 0x666666

ScreenPicker::ScreenPicker(COLORREF color){

	_instance = NULL;
	_parent_window = NULL;
	_mask_window = NULL;

	_old_color = color;
	_new_color = 0;

	_hbrush_old = NULL;
	_hbrush_new = NULL;
	_hbrush_bg = NULL;

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
	
	_cursor = ::LoadCursor(_instance, MAKEINTRESOURCE(IDI_CURSOR));

	WNDCLASSEX wc = {0};
	wc.cbSize        = sizeof(wc);
	wc.lpfnWndProc   = MaskWindowWINPROC;
	wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = _instance;
	wc.hCursor       = _cursor;
	wc.lpszClassName = MASK_WIN_CLASS;

	if (!::RegisterClassEx(&wc)) {
		throw std::runtime_error("ScreenPicker: RegisterClassEx() failed");
	}

	_mask_window = ::CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT, MASK_WIN_CLASS, L"", WS_POPUP, 0, 0, 0, 0, NULL, NULL, _instance, this);

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
		case WM_LBUTTONUP:
		{
			End();
			::SendMessage(_parent_window, WM_SCREEN_PICK_COLOR, 0, (LPARAM)_new_color);
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

void ScreenPicker::Color(COLORREF color){
	_old_color = color;
}

void ScreenPicker::Start(){

	HMONITOR monitor = ::MonitorFromWindow(_parent_window, MONITOR_DEFAULTTONEAREST);
	MONITORINFO mi;
	mi.cbSize = sizeof(MONITORINFO);
	::GetMonitorInfo(monitor, (LPMONITORINFO)&mi);
	int width = mi.rcMonitor.right - mi.rcMonitor.left;
	int height = mi.rcMonitor.bottom - mi.rcMonitor.top;

	::SetWindowPos(_mask_window, HWND_TOPMOST, mi.rcMonitor.left, mi.rcMonitor.top, width, height, SWP_SHOWWINDOW);
	::SetActiveWindow(_mask_window);
}

void ScreenPicker::End(){

	::SetWindowPos(_mask_window, HWND_TOPMOST, 0, 0, 0, 0, SWP_HIDEWINDOW);
	::SetWindowPos(_info_window, HWND_TOP, 0, 0, 0, 0, SWP_HIDEWINDOW);

}

void ScreenPicker::SampleColor(LPARAM lparam){
	
	int x = GET_X_LPARAM(lparam);
	int y = GET_Y_LPARAM(lparam);

	HDC hdc = ::GetDC(HWND_DESKTOP);
	_new_color = ::GetPixel(hdc, x, y);
	::ReleaseDC(HWND_DESKTOP, hdc);

	::SendMessage(_parent_window, WM_SCREEN_HOVER_COLOR, 0, (LPARAM)_new_color);

	// place info window
	HMONITOR monitor = ::MonitorFromWindow(_parent_window, MONITOR_DEFAULTTONEAREST);
	MONITORINFO mi;
	mi.cbSize = sizeof(MONITORINFO);
	::GetMonitorInfo(monitor, (LPMONITORINFO)&mi);

	int win_x = x+20;
	int win_y = y-20-INFO_WINDOW_HEIGHT;
	
	if(win_x + INFO_WINDOW_WIDTH > mi.rcMonitor.right)
		win_x = x-20-INFO_WINDOW_WIDTH;

	if(win_y < mi.rcMonitor.top)
		win_y = y+20;

	::SetWindowPos(_info_window, HWND_TOP, win_x, win_y, INFO_WINDOW_WIDTH, INFO_WINDOW_HEIGHT, SWP_SHOWWINDOW);

	// display color text
	wchar_t color_hex[10];
	wsprintf(color_hex, L"#%06X", _new_color);
	::SetDlgItemText(_info_window, IDC_SCR_COLOR_HEX, color_hex);

	wchar_t color_rgb[20];
	wsprintf(color_rgb, L"%d, %d, %d", GetRValue(_new_color), GetGValue(_new_color), GetBValue(_new_color));
	::SetDlgItemText(_info_window, IDC_SCR_COLOR_RGB, color_rgb);

	// paint preview
	HDC hdc_win = ::GetDC(_info_window);
	RECT rc;
	HBRUSH brush;
	
	HDC hdc_desktop = ::GetDC(HWND_DESKTOP);
	::StretchBlt(hdc_win, 5, 5, 87, 87, hdc_desktop, x-4, y-4, 9, 9, SRCCOPY);
	::ReleaseDC(HWND_DESKTOP, hdc_desktop);

	rc.left = 5;
	rc.top = 5;
	rc.right = 93;
	rc.bottom = 93;
	brush = ::CreateSolidBrush(0);
	::FrameRect(hdc_win, &rc, brush);
	::DeleteObject(brush);

	rc.left = 43;
	rc.top = 43;
	rc.right = 54;
	rc.bottom = 54;
	brush = ::CreateSolidBrush(_new_color ^ 0xffffff);
	::FrameRect(hdc_win, &rc, brush);
	::DeleteObject(brush);

	// paint color swatch
	rc.left = 99;
	rc.top = 6;
	rc.right = 172;
	rc.bottom = 32;
	brush = ::CreateSolidBrush(SWATCH_BG_COLOR);
	::FillRect(hdc_win, &rc, brush);
	::DeleteObject(brush);

	rc.left = 100;
	rc.top = 7;
	rc.right = 136;
	rc.bottom = 31;
	brush = ::CreateSolidBrush(_new_color);
	::FillRect(hdc_win, &rc, brush);
	::DeleteObject(brush);

	rc.left = 136;
	rc.top = 7;
	rc.right = 171;
	rc.bottom = 31;
	brush = ::CreateSolidBrush(_old_color);
	::FillRect(hdc_win, &rc, brush);
	::DeleteObject(brush);

	::ReleaseDC(_info_window, hdc_win);

}


// the information window
void ScreenPicker::CreateInfoWindow(){
			
	_info_window = ::CreateDialogParam(_instance, MAKEINTRESOURCE(IDD_SCREEN_PICKER_POPUP), NULL, (DLGPROC)InfoWindowWINPROC, (LPARAM)this);

	if(!_info_window){
		throw std::runtime_error("ScreenPicker: Create info window failed.");
	}
	
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

	::SetWindowPos(_info_window, HWND_TOP, 0, 0, 0, 0, SWP_HIDEWINDOW);
	::SetWindowLong(_info_window, GWL_EXSTYLE, WS_EX_TOOLWINDOW);

	HWND ctrl;

	ctrl = ::GetDlgItem(_info_window, IDC_SCR_COLOR_RGB);
	::SetWindowPos(ctrl, NULL, 100, INFO_WINDOW_HEIGHT-38, 80, 16, SWP_NOZORDER);

	ctrl = ::GetDlgItem(_info_window, IDC_SCR_COLOR_HEX);
	::SetWindowPos(ctrl, NULL, 100, INFO_WINDOW_HEIGHT-22, 80, 16, SWP_NOZORDER);

}

