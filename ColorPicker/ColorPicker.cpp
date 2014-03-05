

#include "ColorPicker.h"
#include "ScreenPicker.h"

#include <stdio.h>
#include <iostream>
#include <math.h>

#include <windowsx.h>

ColorPicker::ColorPicker(COLORREF color) {
	
	focus_on_show = true;

	_instance = NULL;
	_parent_window = NULL;
	_color_popup = NULL;
	
	_message_window = NULL;

	_pick_cursor = NULL;
	_show_picker_cursor = false;

	_old_color_found_in_palette = false;
	_old_color = color;
	_new_color = 0;	

	_old_color_row = -1;
	_old_color_index = -1;

	_previous_color = NULL;
	_previous_row = -1;
	_previous_index = -1;

	_adjust_color = _old_color;
	_adjust_preserved_hue = 0;
	_adjust_preserved_saturation = 0;

	_adjust_center_row = 0;
	_adjust_row = -1;
	_adjust_index = -1;
	
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
	_adjust_color = _old_color;
	_old_color_found_in_palette = false;

	PaintAll();

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

	int len = RECENT_ZONE_COLUMN*RECENT_ZONE_ROW;
	
	for (int i=0; i<len; i++) {
		if(colors[i]>0xffffff || colors[i]<0)
			continue;
		_recent_color_data[i] = colors[i];
	}

	FillRecentColorData();

}

void ColorPicker::GetRecentColor(COLORREF *&colors){

	for (int i=0; i<RECENT_ZONE_COLUMN*RECENT_ZONE_ROW; i++) {
		colors[i] = _recent_color_data[i];
	}

}

// alter the parent rect to move the color popup
void ColorPicker::SetParentRect(RECT rc) {

	::CopyRect(&_parent_rc, &rc);

	PlaceWindow(_color_popup, rc);

}

// static
// place a window near the parent
void ColorPicker::PlaceWindow(const HWND hwnd, const RECT rc) {

	HMONITOR monitor = ::MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
	MONITORINFO mi;
	mi.cbSize = sizeof(MONITORINFO);
	::GetMonitorInfo(monitor, (LPMONITORINFO)&mi);

	RECT rc_win;
	::GetWindowRect(hwnd, &rc_win);

	int win_width = rc_win.right - rc_win.left;
	int win_height = rc_win.bottom - rc_win.top;

	int x = 0, y = 0;

	if ((rc.left + win_width) > mi.rcWork.right) {
		x = mi.rcWork.right - win_width;
	}else{
		x = rc.left;
	}

	if ((rc.bottom + win_height) > mi.rcWork.bottom) {
		y = rc.top - win_height;
	}else{
		y = rc.bottom;
	}

	::SetWindowPos(hwnd, HWND_TOP, x, y, 0, 0, SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);

}

void ColorPicker::Show() {

	int flags = SWP_SHOWWINDOW|SWP_NOMOVE|SWP_NOSIZE;
	if (!focus_on_show)	{
		flags = flags | SWP_NOACTIVATE;
	}

	::SetWindowPos(_color_popup, HWND_TOP, 0, 0, 0, 0, flags);

	PaintAll();

	DisplayNewColor(_old_color);

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
			if (_show_picker_cursor) {
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
			return OnMouseClick(lparam, false);
		}
		case WM_RBUTTONUP:
		{
			return OnMouseClick(lparam, true);
		}
		case WM_QCP_SCREEN_PICK:
		{
			COLORREF color = (COLORREF)wparam;
			SaveToRecentColor(color);
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

	// set size
	::SetWindowPos(_color_popup, HWND_TOP, 0, 0, POPUP_WIDTH, POPUP_HEIGHT, SWP_HIDEWINDOW|SWP_NOMOVE|SWP_NOACTIVATE);

	// place window
	::GetWindowRect(_color_popup, &_parent_rc);
	PlaceWindow(_color_popup, _parent_rc);

	// dialog control ui stuff
	_pick_cursor = ::LoadCursor(_instance, MAKEINTRESOURCE(IDI_CURSOR));
	HICON hIcon = (HICON)::LoadImage(_instance, MAKEINTRESOURCE(IDI_PICKER), IMAGE_ICON, 16, 16, 0);

	HWND ctrl;
	ctrl = ::GetDlgItem(_color_popup, ID_PICK);
	::MoveWindow(ctrl, BUTTON_X, BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT, false);
	::SendDlgItemMessage(_color_popup, ID_PICK, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcon);

	ctrl = ::GetDlgItem(_color_popup, ID_MORE);
	::MoveWindow(ctrl, BUTTON_X + BUTTON_WIDTH + CONTROL_PADDING, BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT, false);

	ctrl = ::GetDlgItem(_color_popup, IDC_COLOR_TEXT);
	::MoveWindow(ctrl, TEXT_X, TEXT_Y, TEXT_WIDTH, TEXT_HEIGHT, false);

	// generate palette color array
	GenerateColorPaletteData();
	LoadRecentColorData();
	FillRecentColorData();

}

// WM_MOUSEMOVE
BOOL ColorPicker::OnMouseMove(LPARAM lparam){
	
	POINT p;
	p.x = GET_X_LPARAM(lparam);
	p.y = GET_Y_LPARAM(lparam);

	if (PointInRect(p, _rect_palette)) {
		
		// inside palette area
		PaletteMouseMove(p);

	} else if (PointInRect(p, _rect_adjust_buttons)) {

		// inside adjust area
		AdjustZoneMouseMove(p);

	} else {
		
		// outside palette
		_show_picker_cursor = false;

		if (_previous_row != -1) {
			if (_previous_row!=_old_color_row && _previous_index!=_old_color_index) {
				DrawColorHoverBox(_previous_row, _previous_index, false);
			}
			_previous_color = NULL;
			_previous_row = -1;
			_previous_index = -1;
		}

		if (_adjust_row != -1) {
			DrawAdjustZoneHoverBox(_adjust_row, _adjust_index, false);
			_adjust_row = -1;
			_adjust_index = -1;

		}

		DisplayNewColor(_old_color);

	}

	return TRUE;

}


// WM_LBUTTONUP
BOOL ColorPicker::OnMouseClick(LPARAM lparam, bool is_right_button){

	POINT p;
	p.x = GET_X_LPARAM(lparam);
	p.y = GET_Y_LPARAM(lparam);

	if (PointInRect(p, _rect_palette)) {

		PaletteMouseClick(p, is_right_button);

	} else if (PointInRect(p, _rect_adjust_buttons)) {

		AdjustZoneMouseClick(p, is_right_button);

	}

	return TRUE;

}

bool ColorPicker::PointInRect(const POINT p, const RECT rc) {
	
	return p.x>rc.left && p.x<rc.right && p.y>rc.top && p.y<rc.bottom;

}

void ColorPicker::PaintAll(){
	PaintColorPalette();
	PaintColorSwatch();
	PaintAdjustZone();
}

///////////////////////////////////////////////////////////////////
// COLOR SWATCHES
///////////////////////////////////////////////////////////////////

void ColorPicker::PaintColorSwatch() {
	
	// paint swatch /////////
	HDC hdc_win = ::GetDC(_color_popup);
	RECT rc;
	HBRUSH brush;
	
	// new color
	rc.top = SWATCH_Y;
	rc.bottom = rc.top+SWATCH_HEIGHT;
	rc.left = SWATCH_X;
	rc.right = rc.left + SWATCH_WIDTH;
	brush = ::CreateSolidBrush(_new_color);
	::FillRect(hdc_win, &rc, brush);
	::DeleteObject(brush);

	// old color
	rc.left = rc.right;
	rc.left = rc.left + SWATCH_WIDTH;
	brush = ::CreateSolidBrush(_old_color);
	::FillRect(hdc_win, &rc, brush);
	::DeleteObject(brush);
	
	// frame
	rc.left = SWATCH_X;
	rc.right = rc.left + SWATCH_WIDTH*2;
	rc.top = SWATCH_Y;
	rc.bottom = rc.top + SWATCH_HEIGHT;
	::InflateRect(&rc, 1, 1);
	brush = ::CreateSolidBrush(CONTROL_BORDER_COLOR);
	::FrameRect(hdc_win, &rc, brush);
	::DeleteObject(brush);

	::ReleaseDC(_color_popup, hdc_win);
}


// show the new color on popup
void ColorPicker::DisplayNewColor(COLORREF color){

	if(color == _new_color) return;

	_new_color = color;

	// transform order for hex display
	color = RGB(GetBValue(color), GetGValue(color), GetRValue(color));
	
	HSLCOLOR hsl = rgb2hsl(color);
	wchar_t output[60];
	wsprintf(output, L"#%06X   HSL(%d,%d%%,%d%%)", color, round(hsl.h), round(hsl.s*100), round(hsl.l*100));
	::SetDlgItemText(_color_popup, IDC_COLOR_TEXT, output);

	PaintColorSwatch();

}


///////////////////////////////////////////////////////////////////
// COLOR PALETTE
///////////////////////////////////////////////////////////////////
void ColorPicker::GenerateColorPaletteData(){

	int row = 0;

	// generate grayscale - 3 ROWS
	for (int i = 0; i < PALETTE_COLUMN; i++) {

		int t0 = (i-RECENT_ZONE_COLUMN)*0x11;
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
	for (double l = 0.125; l < 1; l += 0.075) {
		for(int i=0; i<24; i++){
			int h = i*15;
			COLORREF rgb = hsl2rgb((double)h, 1.0, l);
			_color_palette_data[row][i] = rgb;
		}
		row++;
	}

}

void ColorPicker::LoadRecentColorData(){

	for(int i=0; i < RECENT_ZONE_ROW*RECENT_ZONE_COLUMN; i++){
		_recent_color_data[i] = 0;
	}

}

void ColorPicker::FillRecentColorData(){

	// fill recent colors into first 2 rows of Color Palette
	for(int i = 0; i < RECENT_ZONE_ROW; i++){
		for(int j = 0; j < RECENT_ZONE_COLUMN; j++){
			_color_palette_data[i][j] = _recent_color_data[j+i*RECENT_ZONE_COLUMN];
		}
	}

}

void ColorPicker::SaveToRecentColor(COLORREF color){

	// shift start at the end
	int pos = RECENT_ZONE_ROW*RECENT_ZONE_COLUMN - 1;

	// whether the color is already inside the list
	for (int i = 0; i < RECENT_ZONE_ROW*RECENT_ZONE_COLUMN; i++) {
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

void ColorPicker::PaintColorPalette(){
	
	RECT rc;

	COLORREF color;
	HBRUSH hbrush;
	HDC hdc = ::GetDC(_color_popup);
	
	// frame
	rc.left = PALETTE_X;
	rc.top =  PALETTE_Y;
	rc.right = PALETTE_X+PALETTE_CELL_SIZE*PALETTE_COLUMN;
	rc.bottom = PALETTE_Y+PALETTE_CELL_SIZE*PALETTE_ROW;
	
	// save the area rect for later use
	::CopyRect(&_rect_palette, &rc);

	::InflateRect(&rc, 1, 1);

	hbrush = ::CreateSolidBrush(CONTROL_BORDER_COLOR);
	::FrameRect(hdc, &rc, hbrush);
	::DeleteObject(hbrush);

	// swatches
	for (int row = 0; row < PALETTE_ROW; row++) {
		
		rc.top = PALETTE_Y + PALETTE_CELL_SIZE*row;
		rc.bottom = rc.top + PALETTE_CELL_SIZE;
		rc.left = PALETTE_X;
		rc.right = rc.left + PALETTE_CELL_SIZE;

		for (int index = 0; index < PALETTE_COLUMN; index++) {

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

			rc.left += PALETTE_CELL_SIZE;
			rc.right += PALETTE_CELL_SIZE;

		}

	}

	::ReleaseDC(_color_popup, hdc);

	// mark current color
	if (_old_color_found_in_palette)
		DrawColorHoverBox(_old_color_row, _old_color_index, true);

}

void ColorPicker::DrawColorHoverBox(int row, int index, bool is_hover) {

	HDC hdc = ::GetDC(_color_popup);
	HBRUSH hbrush;
	RECT rc;
	COLORREF color;

	color = _color_palette_data[row][index];
	if (is_hover)
		color = color ^ 0xffffff;

	rc.left = _rect_palette.left + index*PALETTE_CELL_SIZE;
	rc.right = rc.left + PALETTE_CELL_SIZE;
	rc.top = _rect_palette.top + row*PALETTE_CELL_SIZE;
	rc.bottom = rc.top + PALETTE_CELL_SIZE;
	hbrush = ::CreateSolidBrush(color);
	::FrameRect(hdc, &rc, hbrush);
	::DeleteBrush(hbrush);

	::ReleaseDC(_color_popup, hdc);

}

void ColorPicker::PaletteMouseMove(const POINT p){

	_show_picker_cursor = true;

	int index = round((p.x-_rect_palette.left)/PALETTE_CELL_SIZE);
	int row = round((p.y-_rect_palette.top)/PALETTE_CELL_SIZE);
	if (index<0) index = 0;
	if (row<0) row = 0;

	if (index>=PALETTE_COLUMN || row>=PALETTE_ROW)
		return;

	COLORREF color = _color_palette_data[row][index];

	// no change
	if (_previous_row==row && _previous_index==index)
		return;

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

}

void ColorPicker::PaletteMouseClick(const POINT p, bool is_right_button){

	if(!is_right_button){
		_old_color = _new_color;
		SaveToRecentColor(_old_color);
		::SendMessage(_message_window, WM_QCP_PICK, _old_color, 0);
	}else{
		_adjust_color = _new_color;
		PaintAdjustZone();
	}

}


///////////////////////////////////////////////////////////////////
// ADJUST BUTTONS
///////////////////////////////////////////////////////////////////

void ColorPicker::GenerateAdjustColors(const COLORREF color){

	HSLCOLOR hsl = rgb2hsl(color);
	
	// preserve color hue after converting between rgb & hsl
	if(hsl.s==0){
		hsl.h = _adjust_preserved_hue;
	}else{
		_adjust_preserved_hue = hsl.h;
	}

	// increase the saturation to avoid color lost on extreme lightness
	if(hsl.l==0 || hsl.l==1.0){
		hsl.s = _adjust_preserved_saturation;
	}else{
		_adjust_preserved_saturation = hsl.s;
	}

	double q[] = {20.0, 10.0, 5.0, 2.0, 0 , -2.0, -5.0, -10.0, -20.0};

	for(int i=0; i<ADJUST_ZONE_ROW; i++){
		if(q[i]==0){
			_adjust_center_row = i;
			_adjust_color_data[0][i] = color;
			_adjust_color_data[1][i] = color;
			_adjust_color_data[2][i] = color;
			continue;
		}
		_adjust_color_data[0][i] = hsl2rgb(hsl.h + q[i], hsl.s, hsl.l);
		_adjust_color_data[1][i] = hsl2rgb(hsl.h, hsl.s + q[i]/50, hsl.l);
		_adjust_color_data[2][i] = hsl2rgb(hsl.h, hsl.s, hsl.l + q[i]/100);
	}

}

void ColorPicker::PaintAdjustZone(){

	GenerateAdjustColors(_adjust_color);

	RECT rc;
	int base_x = PALETTE_WIDTH + CONTROL_PADDING*2;
	int base_y = CONTROL_PADDING;

	HBRUSH hbrush;
	HDC hdc = ::GetDC(_color_popup);
	
	// frame
	rc.left = ADJUST_ZONE_X;
	rc.right = rc.left + ADJUST_ZONE_WIDTH;
	rc.top =  ADJUST_ZONE_Y;
	rc.bottom = rc.top + ADJUST_ZONE_HEIGHT;
	
	// save the area rect for later use
	::CopyRect(&_rect_adjust_buttons, &rc);

	::InflateRect(&rc, 1, 1);

	hbrush = ::CreateSolidBrush(CONTROL_BORDER_COLOR);
	::FillRect(hdc, &rc, hbrush);
	::DeleteObject(hbrush);

	// paint current color
	for(int i=0; i<ADJUST_ZONE_ROW; i++){
		rc.top = ADJUST_ZONE_Y + ADJUST_ZONE_CELL_HEIGHT*i;
		rc.bottom = rc.top + ADJUST_ZONE_CELL_HEIGHT;
		rc.left = ADJUST_ZONE_X;
		rc.right = rc.left + ADJUST_ZONE_CELL_WIDTH;
		hbrush = ::CreateSolidBrush(_adjust_color_data[0][i]);
		::FillRect(hdc, &rc, hbrush);
		::DeleteObject(hbrush);
		rc.left = rc.right+1;
		rc.right = rc.left + ADJUST_ZONE_CELL_WIDTH;
		hbrush = ::CreateSolidBrush(_adjust_color_data[1][i]);
		::FillRect(hdc, &rc, hbrush);
		::DeleteObject(hbrush);
		rc.left = rc.right+1;
		rc.right = rc.left + ADJUST_ZONE_CELL_WIDTH;
		hbrush = ::CreateSolidBrush(_adjust_color_data[2][i]);
		::FillRect(hdc, &rc, hbrush);
		::DeleteObject(hbrush);
	}

	rc.left = ADJUST_ZONE_X;
	rc.right = rc.left + ADJUST_ZONE_WIDTH;
	rc.top = ADJUST_ZONE_Y + ADJUST_ZONE_CELL_HEIGHT*_adjust_center_row;
	rc.bottom = rc.top + ADJUST_ZONE_CELL_HEIGHT;

	hbrush = ::CreateSolidBrush(_adjust_color);
	::FillRect(hdc, &rc, hbrush);
	::DeleteObject(hbrush);

	::InflateRect(&rc, 1, 0);
	hbrush = ::CreateSolidBrush(CONTROL_BORDER_COLOR);
	::FrameRect(hdc, &rc, hbrush);
	::DeleteObject(hbrush);

	::ReleaseDC(_color_popup, hdc);

}

void ColorPicker::DrawAdjustZoneHoverBox(int row, int index, bool is_hover){
	
	HDC hdc = ::GetDC(_color_popup);
	HBRUSH hbrush;
	RECT rc;
	COLORREF color;

	color = _adjust_color_data[index][row];
	if (is_hover)
		color = color ^ 0xffffff;

	rc.left = _rect_adjust_buttons.left + index*(ADJUST_ZONE_CELL_WIDTH+1);
	rc.right = rc.left + ADJUST_ZONE_CELL_WIDTH;
	rc.top = _rect_adjust_buttons.top + row*ADJUST_ZONE_CELL_HEIGHT;
	rc.bottom = rc.top + ADJUST_ZONE_CELL_HEIGHT;

	if(row == _adjust_center_row){
		rc.left = _rect_adjust_buttons.left;
		rc.right = _rect_adjust_buttons.right;
		::InflateRect(&rc, 0, -1);
	}

	hbrush = ::CreateSolidBrush(color);
	::FrameRect(hdc, &rc, hbrush);
	::DeleteBrush(hbrush);

	::ReleaseDC(_color_popup, hdc);

}

void ColorPicker::AdjustZoneMouseMove(const POINT p){


	int index = round((p.x-_rect_adjust_buttons.left)/(ADJUST_ZONE_CELL_WIDTH+1));
	int row = round((p.y-_rect_adjust_buttons.top)/ADJUST_ZONE_CELL_HEIGHT);
	if (index<0) index = 0;
	if (row<0) row = 0;

	if(row == _adjust_center_row){
		_show_picker_cursor = true;
	}else{
		_show_picker_cursor = false;
	}

	if (index>=3 || row>=ADJUST_ZONE_ROW)
		return;

	// no change
	if (_adjust_row==row && _adjust_index==index)
		return;

	// clean previous swatch
	if (_adjust_row != -1) {
		DrawAdjustZoneHoverBox(_adjust_row, _adjust_index, false);
	}

	// draw current swatch
	DrawAdjustZoneHoverBox(row, index, true);

	_adjust_row = row;
	_adjust_index = index;

	COLORREF color = _adjust_color_data[index][row];
	DisplayNewColor(color);

}

void ColorPicker::AdjustZoneMouseClick(const POINT p, bool is_right_button){

	if(_adjust_row==-1 || _adjust_index==-1)
		return;

	COLORREF color = _adjust_color_data[_adjust_index][_adjust_row];

	// pick color
	if(_adjust_center_row == _adjust_row){
		_old_color = color;
		SaveToRecentColor(_old_color);
		::SendMessage(_message_window, WM_QCP_PICK, _old_color, 0);
		return;
	}

	// adjust color
	_adjust_color = color;
	PaintAdjustZone();
	DrawAdjustZoneHoverBox(_adjust_row, _adjust_index, true);

	// update preview color
	color = _adjust_color_data[_adjust_index][_adjust_row];
	DisplayNewColor(color);

}


///////////////////////////////////////////////////////////////////
// SCREEN COLOR PICKER
///////////////////////////////////////////////////////////////////

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
	
	PaintAll();

}


///////////////////////////////////////////////////////////////////
// WINDOWS COLOR CHOOSER DIALOG
///////////////////////////////////////////////////////////////////

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

	for(int i=0; i<16; i++){
		custom_colors[i] = _recent_color_data[i];
	}

	::ZeroMemory(&cc, sizeof(cc));
	cc.lStructSize = sizeof(cc);
	cc.hwndOwner = _color_popup;
	cc.lpCustColors = (LPDWORD)custom_colors;
	cc.rgbResult = _old_color;
	cc.Flags = CC_FULLOPEN | CC_RGBINIT | CC_ENABLEHOOK;
	cc.lpfnHook = (LPCCHOOKPROC)ColorChooserWINPROC;
	cc.lCustData = (LPARAM)&_parent_rc;

	Hide();

	_is_color_chooser_shown = true;

	if (ChooseColor(&cc)==TRUE) {
		SaveToRecentColor(cc.rgbResult);
		::SendMessage(_message_window, WM_QCP_PICK, cc.rgbResult, 0);
	} else {
		::SendMessage(_message_window, WM_QCP_CANCEL, 0, 0);
	}

}

UINT_PTR CALLBACK ColorPicker::ColorChooserWINPROC(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam){
	
	if(message == WM_INITDIALOG){
		// reposition the color chooser window
		CHOOSECOLOR* cc = (CHOOSECOLOR*)lparam;
		RECT* rc = (RECT*)cc->lCustData;
		RECT rc_copy;
		::CopyRect(&rc_copy, rc);
		PlaceWindow(hwnd, rc_copy);
	}

	return 0;

}


/////////////////////////////////////////////////////////////////////
// HELPER FUNCTIONS
/////////////////////////////////////////////////////////////////////

ColorPicker::HSLCOLOR ColorPicker::rgb2hsl(COLORREF rgb){
	
	double r, g, b;
	r = (double)GetRValue(rgb) / 255.0;
	g = (double)GetGValue(rgb) / 255.0;
    b = (double)GetBValue(rgb) / 255.0;

    double max, min;
	max = max(max(r, g), b);
	min = min(min(r, g), b);

    double h, s, l, d;
	l = (max + min) / 2.0;
	d = max - min;

    if (max == min) {
        h = s = 0;
    } else {

        s = l > 0.5 ?
				d / (2.0 - max - min)
			:
				d / (max + min);

        if (max == r) {
            h = (g - b) / d + (g < b ? 6.0 : 0);
		}else if(max == g){
            h = (b - r) / d + 2.0;
		}else{
            h = (r - g) / d + 4.0;
        }

        h /= 6.0;

    }

	HSLCOLOR hsl;
	hsl.h = h * 360;
	hsl.s = s;
	hsl.l = l;

	return hsl;

}

COLORREF ColorPicker::hsl2rgb(HSLCOLOR hsl){
	return hsl2rgb(hsl.h, hsl.s, hsl.l);
}

COLORREF ColorPicker::hsl2rgb(double h0, double s0, double l0){
	double h, s, l, m1, m2;
	
	h = ((int)h0 % 360);
	if (h<0) h = 360 + h;
	h = h / 360;
	
	s = s0<0 ? 0 : s0;
	s = s0>1.0 ? 1.0 : s;
	
	l = l0<0 ? 0 : l0;
	l = l0>1.0 ? 1.0 : l;

	m2 = l <= 0.5 ?
			l * (s + 1)
		:
			l + s - l * s;

	m1 = l * 2 - m2;

	int r, g, b;
	r = round( hue(h + 1.0/3, m1, m2) * 255 );
	g = round( hue(h, m1, m2) * 255 );
	b = round( hue(h - 1.0/3, m1, m2) * 255 );

	return RGB(r, g, b);
}

double ColorPicker::hue(double h0, double m1, double m2){
	h0 = h0 < 0 ? h0 + 1 : (h0 > 1 ? h0 - 1 : h0);
	if(h0 * 6 < 1){
		return m1 + (m2 - m1) * h0 * 6;
	} else if (h0 * 2 < 1) {
		return m2;
	} else if (h0 * 3 < 2) {
		return m1 + (m2 - m1) * (2.0/3 - h0) * 6;
	} else {
		return m1;
	}
}

int ColorPicker::round(double number) {

    return (number >= 0) ? (int)(number + 0.5) : (int)(number - 0.5);

}