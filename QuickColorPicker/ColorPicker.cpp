

#include "ColorPicker.h"
#include "ScreenPicker.h"

#include <stdio.h>
#include <iostream>
#include <math.h>

#include <windowsx.h>

using namespace QuickColorPicker;

ColorPicker::ColorPicker() {
	
	RGBAColor color = RGBAColor(0,0,0,1);

	focus_on_show = true;

	_instance = NULL;
	_parent_window = NULL;
	_color_popup = NULL;
	
	_message_window = NULL;

	_pick_cursor = NULL;
	_show_picker_cursor = false;

	_old_color_found_in_palette = false;
	_old_color = color;
	_new_color = color;	

	_old_color_row = -1;
	_old_color_index = -1;

	_previous_color = color;
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


void ColorPicker::SetColor(RGBAColor color) {
	
	_old_color = color;
	_adjust_color = _old_color;
	_old_color_found_in_palette = false;

	GenerateColorPaletteData(color.a);
	FillRecentColorData();

	PaintAll();

}

void ColorPicker::SetHSLAColor(HSLAColor color) {
	RGBAColor rgb = hsl2rgb(color);
	SetColor(rgb);
}

HSLAColor ColorPicker::GetHSLAColor() {
	HSLAColor hsl = rgb2hsl(_old_color);
	return hsl;
}

bool ColorPicker::SetHexColor(const char* hex_str) {

	RGBAColor color;

	bool result = hex2rgb(hex_str, &color);

	if (result) {
		_old_color = color;
		SetColor(_old_color);
	}

	return result;

}



bool ColorPicker::GetHexColor(char* out_hex, int buffer_size) {

	return rgb2hex(_old_color, out_hex, buffer_size);

}


void ColorPicker::SetRecentColor(const RGBAColor *colors){

	int len = RECENT_ZONE_COLUMN*RECENT_ZONE_ROW;
	
	COLORREF rgb;
	RGBAColor color;

	for (int i=0; i<len; i++) {

		color = colors[i];
		rgb = (COLORREF)color;

		if(rgb>0xffffff || rgb<0)
			continue;

		_recent_color_data[i] = colors[i];

	}

	FillRecentColorData();

}

void ColorPicker::GetRecentColor(RGBAColor *&colors){

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

void QuickColorPicker::ColorPicker::ShowScreenPicker()
{
	StartPickScreenColor();
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
			ColorPicker *pColorPicker = reinterpret_cast<ColorPicker*>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
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
			COLORREF rgb = (COLORREF)wparam;
			RGBAColor color = RGBAColor(rgb);
			color.a = 1.0f;
			_old_color = color;
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
			_previous_color = RGBAColor(0,0,0,1);
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
	PaintColorCompareSwatch();
	PaintAdjustZone();
}

// helper function
void ColorPicker::DrawColorBlock(const HDC hdc, const RECT rc, const RGBAColor color, bool ignore_alpha)
{
	HBRUSH hbrush;
	RGBAColor rgb = color;

	if (color.a == 1 || ignore_alpha) {
		hbrush = ::CreateSolidBrush((COLORREF)rgb);
	}
	else {
		COLORREF fgcolor = mixcolor(rgb, RGBAColor(182, 182, 182, 1));
		COLORREF bkcolor = mixcolor(rgb, RGBAColor(255, 255, 255, 1));
		hbrush = ::CreateHatchBrush(HS_FDIAGONAL, fgcolor);
		::SetBkColor(hdc, bkcolor);
	}

	::FillRect(hdc, &rc, hbrush);
	::DeleteObject(hbrush);

}

COLORREF ColorPicker::mixcolor(RGBAColor fgcolor, RGBAColor bkcolor) {
	double r, g, b;
	r = (fgcolor.r * fgcolor.a) + (bkcolor.r * (1.0 - fgcolor.a));
	g = (fgcolor.g * fgcolor.a) + (bkcolor.g * (1.0 - fgcolor.a));
	b = (fgcolor.b * fgcolor.a) + (bkcolor.b * (1.0 - fgcolor.a));
	return RGB(r, g, b);
}

///////////////////////////////////////////////////////////////////
// COLOR SWATCHES
///////////////////////////////////////////////////////////////////

void ColorPicker::PaintColorCompareSwatch() {
	
	// paint swatch /////////
	HDC hdc_win = ::GetDC(_color_popup);
	RECT rc;
	HBRUSH brush;

	// new color
	rc.top = SWATCH_Y;
	rc.bottom = rc.top+SWATCH_HEIGHT;
	rc.left = SWATCH_X;
	rc.right = rc.left + SWATCH_WIDTH;
	DrawColorBlock(hdc_win, rc, _new_color);

	// old color
	rc.left = rc.right;
	rc.left = rc.left + SWATCH_WIDTH;
	DrawColorBlock(hdc_win, rc, _old_color);
	
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
void ColorPicker::DisplayNewColor(RGBAColor color){

	if(color == _new_color) return;

	_new_color = color;

	// prepare data
	char hex[8];
	rgb2hex(color, hex, sizeof(hex));

	HSLAColor hsl = rgb2hsl(color);

	// output
	wchar_t output[80];
	swprintf(output, sizeof(output)/sizeof(wchar_t), L"#%hs / HSLA(%d,%d%%,%d%%,%.2g)", hex, round(hsl.h), round(hsl.s * 100), round(hsl.l * 100), hsl.a);
	::SetDlgItemText(_color_popup, IDC_COLOR_TEXT, output);

	PaintColorCompareSwatch();

}


///////////////////////////////////////////////////////////////////
// COLOR PALETTE
///////////////////////////////////////////////////////////////////
void ColorPicker::GenerateColorPaletteData(float alpha){

	int row = 0;

	// generate grayscale - 3 ROWS
	for (int i = 0; i < PALETTE_COLUMN; i++) {

		int t0 = (i-RECENT_ZONE_COLUMN)*0x11;
		if (t0 < 0) t0 = 0;
	
		int t1 = t0 + 0x02;
		int t1s = t1 - 0x0F;
		if (t1 > 0xff) t1 = 0xff;
		if (t1s < 0) t1 = t1s = 0;
	
		int t2 = t0 - 0x02;
		int t2s = t2 + 0x0F;
		if (t2 < 0) t2 = t2s = 0;
		if (t2s > 0xff) t2s = 0xff;
		
		_color_palette_data[row][i] = RGBAColor(t0, t0, t0, alpha);
		_color_palette_data[row + 1][i] = RGBAColor(t1, t1, t1s, alpha);
		_color_palette_data[row + 2][i] = RGBAColor(t2, t2, t2s, alpha);
	
	}
	
	row += 3;

	// generate web adaptive palette colors
	for (double l = 0.125; l < 1; l += 0.075) {
		for(int i=0; i<24; i++){
			int h = i * 15;
			RGBAColor rgb = hsl2rgb(h, 1.0, l, alpha);
			_color_palette_data[row][i] = rgb;
		}
		row++;
	}

}

void ColorPicker::LoadRecentColorData(){

	for(int i=0; i < RECENT_ZONE_ROW*RECENT_ZONE_COLUMN; i++){
		_recent_color_data[i] = RGBAColor(0,0,0,1);
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

void ColorPicker::SaveToRecentColor(RGBAColor color){

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

	RGBAColor color;
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
			DrawColorBlock(hdc, rc, color);

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
	RGBAColor color;

	color = _color_palette_data[row][index];

	rc.left = _rect_palette.left + index*PALETTE_CELL_SIZE;
	rc.right = rc.left + PALETTE_CELL_SIZE;
	rc.top = _rect_palette.top + row*PALETTE_CELL_SIZE;
	rc.bottom = rc.top + PALETTE_CELL_SIZE;

	if (is_hover) {
		color.r = color.r ^ 0xff;
		color.g = color.g ^ 0xff;
		color.b = color.b ^ 0xff;
		hbrush = ::CreateSolidBrush((COLORREF)color);
		::FrameRect(hdc, &rc, hbrush);
		::DeleteBrush(hbrush);
	}
	else {
		DrawColorBlock(hdc, rc, color);
	}

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

	RGBAColor color = _color_palette_data[row][index];

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
		::SendMessage(_message_window, WM_QCP_PICK, RGB(_old_color.r, _old_color.g, _old_color.b), 0);
	}else{
		_adjust_color = _new_color;
		PaintAdjustZone();
	}

}


///////////////////////////////////////////////////////////////////
// ADJUST BUTTONS
///////////////////////////////////////////////////////////////////

void ColorPicker::GenerateAdjustColors(const RGBAColor color){

	HSLAColor hsl = rgb2hsl(color);
	
	// preserve color hue after converting between rgb & hsl
	if(hsl.s == 0){
		hsl.h = _adjust_preserved_hue;
	}else{
		_adjust_preserved_hue = hsl.h;
	}

	// increase the saturation to avoid color lost on extreme lightness
	if(hsl.l == 0 || hsl.l == 100){
		hsl.s = _adjust_preserved_saturation;
	}else{
		_adjust_preserved_saturation = hsl.s;
	}

	double q[] = {20.0, 10.0, 5.0, 2.0, 1.0, 0, -1.0, -2.0, -5.0, -10.0, -20.0};

	for(int i=0; i<ADJUST_ZONE_ROW; i++){
		if(q[i]==0){
			_adjust_center_row = i;
			_adjust_color_data[0][i] = color;
			_adjust_color_data[1][i] = color;
			_adjust_color_data[2][i] = color;
			_adjust_color_data[3][i] = color;
			continue;
		}
		_adjust_color_data[0][i] = hsl2rgb(hsl.h + q[i], hsl.s, hsl.l, hsl.a);
		_adjust_color_data[1][i] = hsl2rgb(hsl.h, hsl.s + q[i]/100, hsl.l, hsl.a);
		_adjust_color_data[2][i] = hsl2rgb(hsl.h, hsl.s, hsl.l + q[i]/100, hsl.a);
		_adjust_color_data[3][i] = hsl2rgb(hsl.h, hsl.s, hsl.l, hsl.a + (float)q[i] / 100);
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
	RGBAColor color;

	for(int i=0; i<ADJUST_ZONE_ROW; i++){

		rc.top = ADJUST_ZONE_Y + ADJUST_ZONE_CELL_HEIGHT*i;
		rc.bottom = rc.top + ADJUST_ZONE_CELL_HEIGHT;
		rc.left = ADJUST_ZONE_X;
		rc.right = rc.left + ADJUST_ZONE_CELL_WIDTH;
		DrawColorBlock(hdc, rc, _adjust_color_data[0][i], true);

		rc.left = rc.right+1;
		rc.right = rc.left + ADJUST_ZONE_CELL_WIDTH;
		DrawColorBlock(hdc, rc, _adjust_color_data[1][i], true);

		rc.left = rc.right+1;
		rc.right = rc.left + ADJUST_ZONE_CELL_WIDTH;
		DrawColorBlock(hdc, rc, _adjust_color_data[2][i], true);

		rc.left = rc.right+1;
		rc.right = rc.left + ADJUST_ZONE_CELL_WIDTH;
		DrawColorBlock(hdc, rc, _adjust_color_data[3][i]);
		::DeleteObject(hbrush);

	}

	// center row
	DrawAdjustZoneCenterRow(hdc);

	::ReleaseDC(_color_popup, hdc);

}

void ColorPicker::DrawAdjustZoneCenterRow(HDC hdc) {
	RECT rc;
	rc.left = ADJUST_ZONE_X;
	rc.right = rc.left + ADJUST_ZONE_WIDTH - ADJUST_ZONE_CELL_WIDTH;
	rc.top = ADJUST_ZONE_Y + ADJUST_ZONE_CELL_HEIGHT*_adjust_center_row;
	rc.bottom = rc.top + ADJUST_ZONE_CELL_HEIGHT;
	DrawColorBlock(hdc, rc, _adjust_color, true);

	rc.left = rc.right;
	rc.right = rc.right + ADJUST_ZONE_CELL_WIDTH;
	DrawColorBlock(hdc, rc, _adjust_color);

	rc.left = ADJUST_ZONE_X;
	::InflateRect(&rc, 1, 0);
	HBRUSH hbrush = ::CreateSolidBrush(CONTROL_BORDER_COLOR);
	::FrameRect(hdc, &rc, hbrush);
	::DeleteObject(hbrush);
}

void ColorPicker::DrawAdjustZoneHoverBox(int row, int index, bool is_hover){
	
	HDC hdc = ::GetDC(_color_popup);
	HBRUSH hbrush;
	RECT rc;
	RGBAColor color;

	color = _adjust_color_data[index][row];

	rc.left = _rect_adjust_buttons.left + index*(ADJUST_ZONE_CELL_WIDTH+1);
	rc.right = rc.left + ADJUST_ZONE_CELL_WIDTH;
	rc.top = _rect_adjust_buttons.top + row*ADJUST_ZONE_CELL_HEIGHT;
	rc.bottom = rc.top + ADJUST_ZONE_CELL_HEIGHT;


	if (is_hover) {
		if(row == _adjust_center_row){
			rc.left = _rect_adjust_buttons.left;
			rc.right = _rect_adjust_buttons.right;
			::InflateRect(&rc, 0, -1);
		}
		color.r = color.r ^ 0xff;
		color.g = color.g ^ 0xff;
		color.b = color.b ^ 0xff;
		hbrush = ::CreateSolidBrush(color);
		::FrameRect(hdc, &rc, hbrush);
		::DeleteBrush(hbrush);
	}
	else {
		if (row == _adjust_center_row) {
			DrawAdjustZoneCenterRow(hdc);
		}
		else {
			bool solid = (index == 3) ? false : true;
			DrawColorBlock(hdc, rc, color, solid);
		}
	}

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

	if (index>=4 || row>=ADJUST_ZONE_ROW)
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

	RGBAColor color = _adjust_color_data[index][row];
	DisplayNewColor(color);

}

void ColorPicker::AdjustZoneMouseClick(const POINT p, bool is_right_button){

	if(_adjust_row==-1 || _adjust_index==-1)
		return;

	RGBAColor color = _adjust_color_data[_adjust_index][_adjust_row];

	// pick color
	if(_adjust_center_row == _adjust_row){
		_old_color = color;
		SaveToRecentColor(_old_color);
		::SendMessage(_message_window, WM_QCP_PICK, (COLORREF)_old_color, 0);
		return;
	}

	// adjust color
	_adjust_color = color;
	PaintAdjustZone();
	DrawAdjustZoneHoverBox(_adjust_row, _adjust_index, true);

	// update pallete as well
	if (_adjust_index == 3) {
		GenerateColorPaletteData(color.a);
		FillRecentColorData();
		PaintColorPalette();
	}


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

	_pScreenPicker->SetColor(_old_color);
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
		custom_colors[i] = (COLORREF)_recent_color_data[i];
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
		RGBAColor color = RGBAColor(cc.rgbResult);
		_old_color = color;
		SaveToRecentColor(color);
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


bool ColorPicker::hex2rgb(const char* hex_str, RGBAColor* out_color) {

	// check length
	size_t len = strlen(hex_str);
	if (len != 3 && len != 6)
		return false;

	// check hex string
	char hex_copy[16];
	strcpy_s(hex_copy, hex_str);

	char *next_token = NULL;
	bool is_hex = (strtok_s(hex_copy, "0123456789ABCDEFabcdef", &next_token) == NULL);
	if (!is_hex)
		return false;

	strcpy_s(hex_copy, hex_str);

	// pad 3 char hex
	if (len == 3) {
		hex_copy[0] = hex_str[0];
		hex_copy[1] = hex_str[0];
		hex_copy[2] = hex_str[1];
		hex_copy[3] = hex_str[1];
		hex_copy[4] = hex_str[2];
		hex_copy[5] = hex_str[2];
		hex_copy[6] = L'\0';
	}

	// convert to color value - color order is inverted
	COLORREF rgb = strtol(hex_copy, NULL, 16);

	out_color->r = GetBValue(rgb);
	out_color->g = GetGValue(rgb);
	out_color->b = GetRValue(rgb);

	return true;

}

bool ColorPicker::rgb2hex(const RGBAColor rgb, char* out_hex, int buffer_size) {

	// check length
	if (buffer_size < 7) {
		return false;
	}

	COLORREF color = RGB(rgb.b, rgb.g, rgb.r);
	sprintf_s(out_hex, buffer_size, "%06x", color);

	return true;

}

HSLAColor ColorPicker::rgb2hsl(RGBAColor rgb){
	
	double r, g, b, a;
	r = (double)rgb.r / 255.0f;
	g = (double)rgb.g / 255.0f;
    b = (double)rgb.b / 255.0f;

	a = rgb.a;
	a = a<0 ? 0 : a;
	a = a>1.0f ? 1.0f : a;

    double max, min, delta;
	max = max(max(r, g), b);
	min = min(min(r, g), b);
	delta = max - min;

    double h, s, l;
	h = s = 0;
	l = (max + min) / 2.0f;

    if (delta != 0) {

		if (l < 0.5f) {
			s = delta / (max + min);
		}
		else {
			s = delta / (2.0f - max - min);
		}

        if (r == max) {
            h = (g - b) / delta ;
		}else if(g == max){
            h = (b - r) / delta + 2.0;
		}else{
            h = (r - g) / delta + 4.0;
        }

		h = h * 60.0f;
		if (h < 0)
			h += 360.0f;

    }

	return HSLAColor(h, s, l, rgb.a);

}

RGBAColor ColorPicker::hsl2rgb(HSLAColor hsl){
	return hsl2rgb(hsl.h, hsl.s, hsl.l, hsl.a);
}

RGBAColor ColorPicker::hsl2rgb(double h0, double s0, double l0, float a0){

	int r, g, b;
	double h, s, l, a;

	h = (int)h0 % 360 + (h0 - (int)h0);
	if (h<0) h = 360.0f + h;

	s = s0<0 ? 0 : s0;
	s = s0>1.0f ? 1.0f : s;

	l = l0<0 ? 0 : l0;
	l = l0>1.0f ? 1.0f : l;

	a = a0<0 ? 0 : a0;
	a = a0>1.0f ? 1.0f : a;

	if (s == 0)
	{
		r = round(l * 255.0f);
		g = round(l * 255.0f);
		b = round(l * 255.0f);
	}
	else
	{
		double t1, t2;
		double th = h / 360.0f;

		if (l < 0.5f)
		{
			t2 = l * (1.0f + s);
		}
		else
		{
			t2 = (l + s) - (l * s);
		}
		t1 = 2.0f * l - t2;

		double tr, tg, tb;
		tr = th + (1.0f / 3.0f);
		tg = th;
		tb = th - (1.0f / 3.0f);

		tr = calc_color(tr, t1, t2);
		tg = calc_color(tg, t1, t2);
		tb = calc_color(tb, t1, t2);
		r = round(tr * 255.0f);
		g = round(tg * 255.0f);
		b = round(tb * 255.0f);
	}

	return RGBAColor(r, g, b, (float)a);

}

double ColorPicker::calc_color(double c, double t1, double t2){
	if (c < 0) c += 1.0f;
	if (c > 1) c -= 1.0f;
	if (6.0f * c < 1.0f) return t1 + (t2 - t1) * 6.0f * c;
	if (2.0f * c < 1.0f) return t2;
	if (3.0f * c < 2.0f) return t1 + (t2 - t1) * (2.0f / 3.0f - c) * 6.0f;
	return t1;
}

int ColorPicker::round(double number) {

    return (number >= 0) ? (int)(number + 0.5f) : (int)(number - 0.5f);

}