
#include <iostream>
#include <Windowsx.h>

#include "ScreenPicker.h"
#include "ColorPicker.res.h"

#define MASK_WIN_CLASS L"nplus_screen_picker"
#define INFO_WINDOW_WIDTH 224
#define INFO_WINDOW_HEIGHT 140
#define CONTROL_BORDER_COLOR 0x666666
#define IDK_HIDE 101
#define IDK_SLOW_MOUSE 102

using namespace QuickColorPicker;

ScreenPicker::ScreenPicker(COLORREF color){

	_instance = NULL;
	_parent_window = NULL;
	_mask_window = NULL;

	_is_shown = false;

	_old_color = color;
	_new_color = 0;

}

ScreenPicker::~ScreenPicker(){

	::DestroyWindow(_mask_window);
	::DestroyWindow(_info_window);

	::UnregisterClass(MASK_WIN_CLASS, _instance);

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
			ScreenPicker* pScreenPicker = reinterpret_cast<ScreenPicker*>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
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
			int x = GET_X_LPARAM(lparam);
			int y = GET_Y_LPARAM(lparam);
			SampleColor(x, y);

			// NOTE: possible fix for display glitch
			::InvalidateRect(_mask_window, NULL, true);

			return TRUE;
		}
		case WM_LBUTTONUP:
		{
			End();
			::SendMessage(_parent_window, WM_QCP_SCREEN_PICK, (WPARAM)_new_color, 0);
			return TRUE;
		}
		case WM_HOTKEY:
		{
			int key = (int)wparam;

			if (key == IDK_HIDE) {
				End();
			}

			if (key == IDK_SLOW_MOUSE) {
				if (!_slow_mouse) {
					_slow_mouse = true;
					ReduceMouseSpeed();
				}
				else {
					_slow_mouse = false;
					RestoreMouseSpeed();
				}
			}

			return TRUE;
		}
		case WM_RBUTTONUP:
		{
			End();
			::SendMessage(_parent_window, WM_QCP_SCREEN_CANCEL, 0, 0);
			return TRUE;
		}
		default:
		{
			return TRUE;
		}
	}
	
}

void ScreenPicker::SetColor(COLORREF color){
	_old_color = color;
}

void ScreenPicker::Start(){

	HMONITOR monitor = ::MonitorFromWindow(_parent_window, MONITOR_DEFAULTTONEAREST);
	MONITORINFO mi;
	mi.cbSize = sizeof(MONITORINFO);
	::GetMonitorInfo(monitor, (LPMONITORINFO)&mi);
	int width = mi.rcMonitor.right - mi.rcMonitor.left;
	int height = mi.rcMonitor.bottom - mi.rcMonitor.top;

	POINT p;
	::GetCursorPos(&p);
	PlaceInfoWindow(p.x, p.y);
	::SetWindowPos(_info_window, HWND_TOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE);

	::SetWindowPos(_mask_window, HWND_TOPMOST, mi.rcMonitor.left, mi.rcMonitor.top, width, height, SWP_SHOWWINDOW);

	_is_shown = true;

	::RegisterHotKey(_mask_window, IDK_HIDE, 0, VK_ESCAPE);
	::RegisterHotKey(_mask_window, IDK_SLOW_MOUSE, 0, VK_SPACE);

}

void ScreenPicker::End(){

	::ShowWindow(_mask_window, SW_HIDE);
	::ShowWindow(_info_window, SW_HIDE);

	_is_shown = false;
	
	if(_slow_mouse)
		RestoreMouseSpeed();

	::UnregisterHotKey(_mask_window, IDK_HIDE);
	::UnregisterHotKey(_mask_window, IDK_SLOW_MOUSE);

}

void ScreenPicker::SampleColor(int x, int y){

	// avoid redundant redraw
	if(!_is_shown)
		return;

	HDC hdc = ::GetDC(HWND_DESKTOP);
	_new_color = ::GetPixel(hdc, x, y);
	::ReleaseDC(HWND_DESKTOP, hdc);

	// notify parent
	::SendMessage(_parent_window, WM_QCP_SCREEN_HOVER, (WPARAM)_new_color, 0);

	// place info window
	PlaceInfoWindow(x, y);

	// display color text
	wchar_t color_hex[10];
	swprintf_s(color_hex, L"#%06x", _new_color);
	::SetDlgItemText(_info_window, IDC_SCR_COLOR_HEX, color_hex);

	wchar_t color_rgb[20];
	swprintf_s(color_rgb, L"%d, %d, %d", GetRValue(_new_color), GetGValue(_new_color), GetBValue(_new_color));
	::SetDlgItemText(_info_window, IDC_SCR_COLOR_RGB, color_rgb);

	// paint preview /////////////
	HDC hdc_win = ::GetDC(_info_window);
	RECT rc;
	HBRUSH brush;
	
	// zoomed preview
	HDC hdc_desktop = ::GetDC(HWND_DESKTOP);
	::StretchBlt(hdc_win, 5, 5, 127, 127, hdc_desktop, x-4, y-4, 9, 9, SRCCOPY);
	::ReleaseDC(HWND_DESKTOP, hdc_desktop);

	// frame box
	rc.left = 5;
	rc.top = 5;
	rc.right = 132;
	rc.bottom = 132;
	brush = ::CreateSolidBrush(0);
	::FrameRect(hdc_win, &rc, brush);
	::DeleteObject(brush);

	// center box
	rc.left = 63;
	rc.top = 63;
	rc.right = 74;
	rc.bottom = 74;
	brush = ::CreateSolidBrush(_new_color ^ 0xffffff);
	::FrameRect(hdc_win, &rc, brush);
	::DeleteObject(brush);

	// paint color swatch /////////////////
	// bg
	rc.left = 139;
	rc.top = 6;
	rc.right = 217;
	rc.bottom = 39;
	brush = ::CreateSolidBrush(CONTROL_BORDER_COLOR);
	::FillRect(hdc_win, &rc, brush);
	::DeleteObject(brush);

	// new color
	rc.left = 140;
	rc.top = 7;
	rc.right = 178;
	rc.bottom = 38;
	brush = ::CreateSolidBrush(_new_color);
	::FillRect(hdc_win, &rc, brush);
	::DeleteObject(brush);

	// old color
	rc.left = 178;
	rc.top = 7;
	rc.right = 216;
	rc.bottom = 38;
	brush = ::CreateSolidBrush(_old_color);
	::FillRect(hdc_win, &rc, brush);
	::DeleteObject(brush);

	::ReleaseDC(_info_window, hdc_win);

}


void QuickColorPicker::ScreenPicker::ReduceMouseSpeed()
{
	// save mouse speed
	int *ms = &_mouse_speed;
	BOOL result = SystemParametersInfo(SPI_GETMOUSESPEED, 0, ms, 0);

	// slow it down
	SystemParametersInfo(SPI_SETMOUSESPEED,
		0,
		(LPVOID)1,
		SPIF_UPDATEINIFILE ||
		SPIF_SENDCHANGE ||
		SPIF_SENDWININICHANGE);

}

void QuickColorPicker::ScreenPicker::RestoreMouseSpeed()
{
	SystemParametersInfo(SPI_SETMOUSESPEED,
		0,
		(LPVOID)_mouse_speed,
		SPIF_UPDATEINIFILE ||
		SPIF_SENDCHANGE ||
		SPIF_SENDWININICHANGE);
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
			ScreenPicker *pScreenPicker = reinterpret_cast<ScreenPicker*>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
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
		case WM_MOUSEMOVE:
		{
			POINT p;
			p.x = GET_X_LPARAM(lparam);
			p.y = GET_Y_LPARAM(lparam);
			::ClientToScreen(_info_window, &p);
			SampleColor(p.x, p.y);
			return TRUE;
		}
		default:
		{
			return FALSE;
		}
	}
	
}


void ScreenPicker::PrepareInfoWindow(){

	::SetWindowPos(_info_window, HWND_TOP, 0, 0, INFO_WINDOW_WIDTH, INFO_WINDOW_HEIGHT, SWP_HIDEWINDOW);
	::SetWindowLongPtr(_info_window, GWL_EXSTYLE, WS_EX_TOOLWINDOW);

	HWND ctrl;

	ctrl = ::GetDlgItem(_info_window, IDC_SCR_COLOR_RGB);
	::MoveWindow(ctrl, 138, INFO_WINDOW_HEIGHT-22, 80, 16, false);

	ctrl = ::GetDlgItem(_info_window, IDC_SCR_COLOR_HEX);
	::MoveWindow(ctrl, 138, INFO_WINDOW_HEIGHT-38, 80, 16, false);

}

void ScreenPicker::PlaceInfoWindow(int x, int y) {

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

	::MoveWindow(_info_window, win_x, win_y, INFO_WINDOW_WIDTH, INFO_WINDOW_HEIGHT, false);


}