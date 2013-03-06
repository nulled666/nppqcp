

#include "ColorPicker.h"
#include "ScreenPicker.h"

#include <stdio.h>
#include <iostream>
#include <math.h>

#include <windowsx.h>


#define POPUP_WIDTH 266
#define POPUP_HEIGHT 220
#define SWATCH_BG_COLOR 0x666666

ColorPicker::ColorPicker(COLORREF color) {
	
	_instance = NULL;
	_parent_window = NULL;
	_color_popup = NULL;
	
	_message_window = NULL;

	_pick_cursor = NULL;

	_old_color_found_in_palette = false;
	_old_color = color;
	_new_color = 0;
	
	_is_inside_palette = false;

	_old_color_row = -1;
	_old_color_index = -1;

	_previous_color = NULL;
	_previous_row = -1;
	_previous_index = -1;
	
	_is_first_create = false;
	_is_color_chooser_shown = false;

	_pScreenPicker = NULL;

}

ColorPicker::~ColorPicker() {

	::DestroyWindow(_color_popup);

}

///////////////////////////////////////////////////////////////////
// GENERAL ROUTINES
///////////////////////////////////////////////////////////////////
void ColorPicker::Create(HINSTANCE instance, HWND parent, HWND message_window) {

	if (IsCreated()) {
		throw std::runtime_error("ColorPicker::Create() : duplicate creation");
	}

	_instance = instance;
	_parent_window = parent;

	_is_first_create = true;

	_color_popup = ::CreateDialogParam(_instance, MAKEINTRESOURCE(IDD_COLOR_PICKER_POPUP), _parent_window, (DLGPROC)ColorPopupWinproc, (LPARAM)this);
	
	if (message_window == NULL) {
		_message_window = _parent_window;
	} else {
		_message_window = message_window;
	}

	if (!_color_popup) {
		throw std::runtime_error("ColorPicker::Create() : CreateDialogParam() function returns null");
	}
	
}


void ColorPicker::Color(COLORREF color, bool is_rgb) {
	
	if (is_rgb) {
		color = RGB(GetBValue(color), GetGValue(color), GetRValue(color));
	}
	
	_old_color = color;
	_old_color_found_in_palette = false;

	// redraw palette
	DrawColorPalette();
	PaintColorSwatches();

}

bool ColorPicker::SetHexColor(const wchar_t* hex_color) {

	// check length
	size_t len = wcslen(hex_color);
	if(len != 3 && len != 6)
		return false;

	// check hex string
	wchar_t hex_copy[16];
	wcscpy(hex_copy, hex_color);
	bool is_hex = (wcstok(hex_copy, L"0123456789ABCDEFabcdef") == NULL);
	if(!is_hex)
		return false;

	wcscpy(hex_copy, hex_color);

	// pad 3 char hex
	if (len==3) {
		hex_copy[0] = hex_color[0];
		hex_copy[1] = hex_color[0];
		hex_copy[2] = hex_color[1];
		hex_copy[3] = hex_color[1];
		hex_copy[4] = hex_color[2];
		hex_copy[5] = hex_color[2];
		hex_copy[6] = L'\0';
	}

	// convert to color value - color order is invert to COLORREF
	COLORREF rgb = wcstol(hex_copy, NULL, 16);

	Color(rgb, true);

	return true;

}

void ColorPicker::SetRecentColor(const COLORREF *colors){

	for (int i=0; i<10; i++) {
		if(colors[i]>0xffffff || colors[i]<0)
			continue;
		_recent_color_data[i] = colors[i];
	}

	FillRecentColorData();

}

void ColorPicker::GetRecentColor(COLORREF *&colors){

	for (int i=0; i<10; i++) {
		colors[i] = _recent_color_data[i];
	}

}

// alter the parent rect to move the color popup
void ColorPicker::SetParentRect(RECT rc) {

	HMONITOR monitor = ::MonitorFromWindow(_color_popup, MONITOR_DEFAULTTONEAREST);
	MONITORINFO mi;
	mi.cbSize = sizeof(MONITORINFO);
	::GetMonitorInfo(monitor, (LPMONITORINFO)&mi);

	int x = 0, y = 0;

	if ((rc.left + POPUP_WIDTH) > mi.rcWork.right) {
		x = mi.rcWork.right - POPUP_WIDTH;
	}else{
		x = rc.left;
	}

	if ((rc.bottom + POPUP_HEIGHT) > mi.rcWork.bottom) {
		y = rc.top - POPUP_HEIGHT;
	}else{
		y = rc.bottom;
	}

    ::SetWindowPos(_color_popup, HWND_TOP, x, y, POPUP_WIDTH, POPUP_HEIGHT, SWP_SHOWWINDOW);

}

void ColorPicker::Show() {

	DrawColorPalette();
	PaintColorSwatches();
	::ShowWindow(_color_popup, SW_SHOW);

}

void ColorPicker::Hide() {

	::ShowWindow(_color_popup, SW_HIDE);

}

BOOL CALLBACK ColorPicker::ColorPopupWinproc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {

	switch (message) {
		case WM_INITDIALOG:
		{
			ColorPicker *pColorPicker = (ColorPicker*)(lparam);
			pColorPicker->_color_popup = hwnd;
			::SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)lparam);
			pColorPicker->ColorPopupMessageHandle(message, wparam, lparam);
			return TRUE;
		}
		case WM_MEASUREITEM:
		{
			LPMEASUREITEMSTRUCT lpmis = (LPMEASUREITEMSTRUCT)lparam; 
			lpmis->itemHeight = 12; 
			lpmis->itemWidth = 12;
			return TRUE;
		}
		default:
		{
			ColorPicker *pColorPicker = reinterpret_cast<ColorPicker*>(::GetWindowLongPtr(hwnd, GWL_USERDATA));
			if (!pColorPicker)
				return FALSE;
			return pColorPicker->ColorPopupMessageHandle(message, wparam, lparam);
		}
	}

}

BOOL CALLBACK ColorPicker::ColorPopupMessageHandle(UINT message, WPARAM wparam, LPARAM lparam){

	switch (message) {
		case WM_INITDIALOG:
		{
			OnInitDialog();
			return TRUE;
		}
		case WM_SETCURSOR:
		{
			if (_is_inside_palette) {
				::SetCursor(_pick_cursor);
				return TRUE;
			}
			return FALSE;
		}
		case WM_MOUSEMOVE:
		{
			return OnMouseMove(lparam);
		}
		case WM_LBUTTONUP:
		{
			return OnMouseClick(lparam);
		}
		case WM_QCP_SCREEN_PICK:
		{
			COLORREF color = (COLORREF)wparam;
			PutRecentColor(color);
			::SendMessage(_message_window, WM_QCP_END_SCREEN_PICKER, 0, 0);
			::SendMessage(_message_window, WM_QCP_PICK, wparam, 0);
			return TRUE;
		}
		case WM_QCP_SCREEN_CANCEL:
		{
			EndPickScreenColor();
			return TRUE;
		}
		case WM_COMMAND:
		{
			switch (LOWORD(wparam)) {
				case ID_PICK: {
					StartPickScreenColor();
					return TRUE;
				}
                case ID_MORE: {
			    	ShowColorChooser();
					return TRUE;
				}
                default: {
                    return FALSE;
				}
            }
		}
		case WM_ACTIVATE:
		{
			if (LOWORD(wparam) == WA_INACTIVE) {
				if (_is_first_create) {
					// prevent this message at first create
					_is_first_create = false;
					return TRUE;
				}
				if (!_is_color_chooser_shown) {
					::SendMessage(_message_window, WM_QCP_DEACTIVATE, 0, 0);
				}
			}
			return TRUE;
		}
	}
	
	return FALSE;

}



///////////////////////////////////////////////////////////////////
// MESSAGE HANDLES
///////////////////////////////////////////////////////////////////

// WM_INITDIALOG
void ColorPicker::OnInitDialog(){

	// dialog control ui stuff
	_pick_cursor = ::LoadCursor(_instance, MAKEINTRESOURCE(IDI_CURSOR));
	HICON hIcon = (HICON)::LoadImage(_instance, MAKEINTRESOURCE(IDI_PICKER), IMAGE_ICON, 16, 16, 0);

	HWND ctrl;
	ctrl = ::GetDlgItem(_color_popup, ID_PICK);
	::MoveWindow(ctrl, 6, 184, 32, 28, false);
	::SendDlgItemMessage(_color_popup, ID_PICK, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcon);

	ctrl = ::GetDlgItem(_color_popup, ID_MORE);
	::MoveWindow(ctrl, 43, 184, 32, 28, false);

	ctrl = ::GetDlgItem(_color_popup, IDC_COLOR_TEXT);
	::MoveWindow(ctrl, 100, 191, 80, 20, false);

	// generate palette color array
	GenerateColorPaletteData();
	LoadRecentColorData();
	FillRecentColorData();

}

void ColorPicker::DrawColorPalette(){
	
	RECT rc;
	int base_x = 6;
	int base_y = 6;

	COLORREF color;
	HBRUSH hbrush;
	HDC hdc = ::GetDC(_color_popup);
	
	// frame
	rc.left = base_x;
	rc.top =  base_y;
	rc.right = base_x+12*21;
	rc.bottom = base_y+12*14;
	
	// save the area rect for later use
	::CopyRect(&_rect_palette, &rc);

	::InflateRect(&rc, 1, 1);

	hbrush = ::CreateSolidBrush(SWATCH_BG_COLOR);
	::FrameRect(hdc, &rc, hbrush);
	::DeleteObject(hbrush);

	// swatches
	for (int row = 0; row < 14; row++) {
		
		rc.top = base_y + 12*row;
		rc.bottom = rc.top + 12;
		rc.left = base_x;
		rc.right = rc.left + 12;

		for (int index = 0; index < 21; index++) {

			color = _color_palette_data[row][index];
			hbrush = ::CreateSolidBrush(color);
			::FillRect(hdc, &rc, hbrush);
			::DeleteObject(hbrush);

			if (!_old_color_found_in_palette && color == _old_color) {
				_old_color_index = index;
				_old_color_row = row;
				if(row>2 || index>4)
					_old_color_found_in_palette = true;
			}

			rc.left += 12;
			rc.right += 12;

		}

	}

	::ReleaseDC(_color_popup, hdc);

	// mark current color
	if (_old_color_found_in_palette)
		DrawColorHoverBox(_old_color_row, _old_color_index, true);

}

// WM_MOUSEMOVE
BOOL ColorPicker::OnMouseMove(LPARAM lparam){
	
	POINT p;
	p.x = GET_X_LPARAM(lparam);
	p.y = GET_Y_LPARAM(lparam);

	if (PointInRect(p, _rect_palette)) {
		
		// inside palette area

		_is_inside_palette = true;

		int index = round((p.x-_rect_palette.left)/12);
		int row = round((p.y-_rect_palette.top)/12);
		if (index<0) index = 0;
		if (row<0) row = 0;

		if (index>21 || row>14)
			return TRUE;

		COLORREF color = _color_palette_data[row][index];

		// no change
		if (_previous_row==row && _previous_index==index)
			return TRUE;

		// clean previous swatch
		if (_previous_row != -1 && (_previous_row!=_old_color_row || _previous_index!=_old_color_index)) {
			DrawColorHoverBox(_previous_row, _previous_index, false);
		}

		// draw current swatch
		DrawColorHoverBox(row, index, true);

		_previous_color = color;
		_previous_row = row;
		_previous_index = index;

		DisplayNewColor(color);

	} else {
		
		// outside palette
		_is_inside_palette = false;

		if (_previous_row != -1) {
			if (_previous_row!=_old_color_row && _previous_index!=_old_color_index) {
				DrawColorHoverBox(_previous_row, _previous_index, false);
			}
			_previous_color = NULL;
			_previous_row = -1;
			_previous_index = -1;
		}

		DisplayNewColor(_old_color);

	}

	return TRUE;

}

void ColorPicker::DrawColorHoverBox(int row, int index, bool is_hover) {

	HDC hdc = ::GetDC(_color_popup);
	HBRUSH hbrush;
	RECT rc;
	COLORREF color;

	color = _color_palette_data[row][index];
	if (is_hover)
		color = color ^ 0xffffff;

	rc.left = _rect_palette.left + index*12;
	rc.right = rc.left + 12;
	rc.top = _rect_palette.top + row*12;
	rc.bottom = rc.top + 12;
	hbrush = ::CreateSolidBrush(color);
	::FrameRect(hdc, &rc, hbrush);
	::DeleteBrush(hbrush);

	::ReleaseDC(_color_popup, hdc);

}

// WM_LBUTTONUP
BOOL ColorPicker::OnMouseClick(LPARAM lparam){

	POINT p;
	p.x = GET_X_LPARAM(lparam);
	p.y = GET_Y_LPARAM(lparam);

	if (PointInRect(p, _rect_palette)) {

		_old_color = _new_color;

		PutRecentColor(_new_color);

		::SendMessage(_message_window, WM_QCP_PICK, _old_color, 0);

	}

	return TRUE;

}

bool ColorPicker::PointInRect(const POINT p, const RECT rc) {
	
	return p.x>rc.left && p.x<rc.right && p.y>rc.top && p.y<rc.bottom;

}


// WM_COMMAND -> ID_PICK
// start the screen picker
void ColorPicker::StartPickScreenColor(){

	::ShowWindow(_color_popup, SW_HIDE);

	::SendMessage(_message_window, WM_QCP_START_SCREEN_PICKER, 0, 0);

	if (!_pScreenPicker) {
		_pScreenPicker = new ScreenPicker();
		_pScreenPicker->Create(_instance, _color_popup);
	}

	_pScreenPicker->Color(_old_color);
	_pScreenPicker->Start();

}

void ColorPicker::EndPickScreenColor(){

	::SendMessage(_message_window, WM_QCP_END_SCREEN_PICKER, 0, 0);
	
	::ShowWindow(_color_popup, SW_SHOW);
	DrawColorPalette();
	PaintColorSwatches();

}


// WM_COMMAND -> ID_MORE
// show the native color chooser
void ColorPicker::ShowColorChooser(){

	// Initialize CHOOSECOLOR 
	CHOOSECOLOR cc;
	static COLORREF custom_colors[16] = {
		0,0,0,0,\
		0,0,0,0,\
		0,0,0,0,\
		0,0,0,0,\
	};

	for(int i=0; i<10; i++){
		custom_colors[i] = _recent_color_data[i];
	}

	::ZeroMemory(&cc, sizeof(cc));
	cc.lStructSize = sizeof(cc);
	cc.hwndOwner = _color_popup;
	cc.lpCustColors = (LPDWORD)custom_colors;
	cc.rgbResult = _old_color;
	cc.Flags = CC_FULLOPEN | CC_RGBINIT | CC_ENABLEHOOK;
	cc.lpfnHook = (LPCCHOOKPROC)ColorChooserWINPROC;

	Hide();

	_is_color_chooser_shown = true;

	if (ChooseColor(&cc)==TRUE) {
		PutRecentColor(cc.rgbResult);
		::SendMessage(_message_window, WM_QCP_PICK, cc.rgbResult, 0);
	} else {
		::SendMessage(_message_window, WM_QCP_CANCEL, 0, 0);
	}

}

UINT_PTR CALLBACK ColorPicker::ColorChooserWINPROC(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam){
	
	if(message == WM_INITDIALOG){
		// reposition the color chooser window
		HWND popup = ::GetParent(hwnd);
		RECT rc_popup, rc_self;
		::GetWindowRect(popup, &rc_popup);
		::GetWindowRect(hwnd, &rc_self);
		::SetWindowPos(hwnd, HWND_TOP, rc_popup.left, rc_popup.top, rc_self.right - rc_self.left, rc_self.bottom - rc_self.top, SWP_SHOWWINDOW);
	}

	return 0;

}


// show the new color on popup
void ColorPicker::DisplayNewColor(COLORREF color){

	if(color == _new_color) return;

	_new_color = color;


	// transform order for hex display
	color = RGB(GetBValue(color), GetGValue(color), GetRValue(color));
	
	wchar_t hex[8];
	wsprintf(hex, L"#%06X", color);
	::SetDlgItemText(_color_popup, IDC_COLOR_TEXT, hex);

	PaintColorSwatches();

}


///////////////////////////////////////////////////////////////////
// COLOR PALETTE
///////////////////////////////////////////////////////////////////
 
void ColorPicker::PaintColorSwatches() {
	
	// paint swatch /////////
	HDC hdc_win = ::GetDC(_color_popup);
	RECT rc;
	HBRUSH brush;
	
	// frame
	rc.left = 208;
	rc.top = 186;
	rc.right = 208+50;
	rc.bottom = 186+24;
	brush = ::CreateSolidBrush(SWATCH_BG_COLOR);
	::FillRect(hdc_win, &rc, brush);
	::DeleteObject(brush);
	
	// new color
	rc.left = 209;
	rc.top = 187;
	rc.right = 209+24;
	rc.bottom = 187+22;
	brush = ::CreateSolidBrush(_new_color);
	::FillRect(hdc_win, &rc, brush);
	::DeleteObject(brush);

	// old color
	rc.left = 233;
	rc.top = 187;
	rc.right = 233+24;
	rc.bottom = 187+22;
	brush = ::CreateSolidBrush(_old_color);
	::FillRect(hdc_win, &rc, brush);
	::DeleteObject(brush);

	::ReleaseDC(_color_popup, hdc_win);
}



/////////////////////////////////////////////////////////////////////
// palette generation
void ColorPicker::PutRecentColor(COLORREF color){

	// shift start at the end
	int pos = 9;

	// whether the color is already inside the list
	for (int i = 0; i < 10; i++) {
		if (_recent_color_data[i] == color) {
			// already first one
			if (i==0) return;
			// found in the middle - set shift start
			pos = i;
			break;
		}
	}

	// shift the array right
	for (int i = pos; i > 0; i--) {
		_recent_color_data[i] = _recent_color_data[i-1];
	}

	// put our color first
	_recent_color_data[0] = color;

	FillRecentColorData();

}

void ColorPicker::LoadRecentColorData(){

	for(int i=0; i < 10; i++){
		_recent_color_data[i] = 0;
	}

}

void ColorPicker::FillRecentColorData(){

	// fill recent colors into first 2 rows of Color Palette
	for(int i = 0; i < 5 ; i++){
		_color_palette_data[0][i] = _recent_color_data[i];
		_color_palette_data[1][i] = _recent_color_data[i+5];
	}

}

void ColorPicker::GenerateColorPaletteData(){

	int row = 0;

	// generate grayscale - 3 ROWS
	for (int i = 0; i < 21; i++) {

		int t0 = (i-5)*0x11;
		if (t0 < 0) t0 = 0;
		if (t0 > 0xFF) t0 = 0xFF;
	
		int t1 = t0 + 0x02;
		int t1s = t1 - 0x0F;
		if (t1 > 0xff) t1 = 0xff;
		if (t1s < 0) t1 = t1s = 0;
	
		int t2 = t0 - 0x02;
		int t2s = t2 + 0x0F;
		if (t2 < 0) t2 = t2s = 0;
		if (t2s > 0xff) t2s = 0xff;
		
		_color_palette_data[row][i] = RGB(t0, t0, t0);
		_color_palette_data[row + 1][i] = RGB(t1, t1, t1s);
		_color_palette_data[row + 2][i] = RGB(t2, t2, t2s);
	
	}
	
	row += 3;

	// generate colors
	for (int i = 0; i < (14 - 3); i ++) {

		int main = 0x40 + round((double)(0xBF/5)*i);
		int main0 = 0;
	
		int side1 = (int)floor((double)main/4);
		int side2 = side1 * 2;
		int side3 = side1 * 3;
	
		if(main >= 0xFF){
			main0 = main - 0xFF;
			main = 0xFF;
			side1 = 0x40 + round((0x8F/5)*(i-5));
			side3 = 0xBF + round((0x30/5)*(i-5));
		}
	
		// do it the stupid way
		#define PUT_COLOR(r,g,b) \
			_color_palette_data[row][index] = RGB(r,g,b); \
			index++;

		int index = 0;

		PUT_COLOR(main, main0, main0 );
		PUT_COLOR(main, side1, main0);
		PUT_COLOR(main, side2, main0);
		PUT_COLOR(main, side3, main0);
		PUT_COLOR(main, main, main0);
		PUT_COLOR(side3, main, main0);
		PUT_COLOR(side2, main, main0);
		PUT_COLOR(side1, main, main0);
		PUT_COLOR(main0, main, main0);
		PUT_COLOR(main0, main, side1);
		PUT_COLOR(main0, main, side2);
		PUT_COLOR(main0, main, side3);
		PUT_COLOR(main0, main, main);
		PUT_COLOR(main0, side3, main);
		PUT_COLOR(main0, side2, main);
		PUT_COLOR(main0, side1, main);
		PUT_COLOR(main0, main0, main);
		PUT_COLOR(side1, main0, main);
		PUT_COLOR(side2, main0, main);
		PUT_COLOR(side3, main0, main);
		PUT_COLOR(main, main0, main);
				
		row++;

	}

}

int ColorPicker::round(double number) {

    return (number >= 0) ? (int)(number + 0.5) : (int)(number - 0.5);

}