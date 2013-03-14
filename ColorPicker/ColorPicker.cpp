

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
	PaintColorPalette();
	PaintColorPreview();

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

	PaintColorPalette();
	PaintColorPreview();
	PaintAdjustButtons();

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
			return OnMouseClick(lparam);
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
	::MoveWindow(ctrl, BUTTON_X + BUTTON_WIDTH*2 + CONTROL_PADDING*2, BUTTON_Y + (BUTTON_HEIGHT-12)/2, 80, 20, false);

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

		DisplayNewColor(_old_color);

	}

	return TRUE;

}


// WM_LBUTTONUP
BOOL ColorPicker::OnMouseClick(LPARAM lparam){

	POINT p;
	p.x = GET_X_LPARAM(lparam);
	p.y = GET_Y_LPARAM(lparam);

	if (PointInRect(p, _rect_palette)) {

		PaletteMouseClick(p);

	}

	return TRUE;

}

bool ColorPicker::PointInRect(const POINT p, const RECT rc) {
	
	return p.x>rc.left && p.x<rc.right && p.y>rc.top && p.y<rc.bottom;

}


///////////////////////////////////////////////////////////////////
// COLOR SWATCHES
///////////////////////////////////////////////////////////////////

void ColorPicker::PaintColorPreview() {
	
	// paint swatch /////////
	HDC hdc_win = ::GetDC(_color_popup);
	RECT rc;
	HBRUSH brush;

	// old color
	rc.right = POPUP_WIDTH - CONTROL_PADDING - 1;
	rc.left = rc.right-SWATCH_WIDTH - 1;
	rc.top = BUTTON_Y;
	rc.bottom = rc.top+SWATCH_HEIGHT;
	brush = ::CreateSolidBrush(_old_color);
	::FillRect(hdc_win, &rc, brush);
	::DeleteObject(brush);
	
	// new color
	rc.right = rc.left;
	rc.left = rc.right-SWATCH_WIDTH;
	brush = ::CreateSolidBrush(_new_color);
	::FillRect(hdc_win, &rc, brush);
	::DeleteObject(brush);
	
	// frame
	rc.left = POPUP_WIDTH - CONTROL_PADDING - SWATCH_WIDTH*2 - 2;
	rc.right = POPUP_WIDTH - CONTROL_PADDING - 1;
	rc.top = BUTTON_Y;
	rc.bottom = BUTTON_Y + SWATCH_HEIGHT;
	brush = ::CreateSolidBrush(SWATCH_BG_COLOR);
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
	
	wchar_t hex[8];
	wsprintf(hex, L"#%06X", color);
	::SetDlgItemText(_color_popup, IDC_COLOR_TEXT, hex);

	PaintColorPreview();

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
	int base_x = CONTROL_PADDING;
	int base_y = CONTROL_PADDING;

	COLORREF color;
	HBRUSH hbrush;
	HDC hdc = ::GetDC(_color_popup);
	
	// frame
	rc.left = base_x;
	rc.top =  base_y;
	rc.right = base_x+PALETTE_CELL_SIZE*PALETTE_COLUMN;
	rc.bottom = base_y+PALETTE_CELL_SIZE*PALETTE_ROW;
	
	// save the area rect for later use
	::CopyRect(&_rect_palette, &rc);

	::InflateRect(&rc, 1, 1);

	hbrush = ::CreateSolidBrush(SWATCH_BG_COLOR);
	::FrameRect(hdc, &rc, hbrush);
	::DeleteObject(hbrush);

	// swatches
	for (int row = 0; row < PALETTE_ROW; row++) {
		
		rc.top = base_y + PALETTE_CELL_SIZE*row;
		rc.bottom = rc.top + PALETTE_CELL_SIZE;
		rc.left = base_x;
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

void ColorPicker::PaletteMouseClick(const POINT p){

	_old_color = _new_color;

	SaveToRecentColor(_new_color);

	::SendMessage(_message_window, WM_QCP_PICK, _old_color, 0);

}


///////////////////////////////////////////////////////////////////
// ADJUST BUTTONS
///////////////////////////////////////////////////////////////////

void ColorPicker::GenerateAdjustColors(const COLORREF color){

	HSLCOLOR hsl = rgb2hsl(color);
	COLORREF cc = hsl2rgb(hsl);

	double q[] = {-20.0, -10.0, -5.0, -1.0, 0 , 1.0, 5.0, 10.0, 20.0};

	for(int i=0; i<ADJUST_BUTTON_ROW; i++){
		if(q[i]==0){
			_adjust_color_data[0][i] = color;
			_adjust_color_data[1][i] = color;
			_adjust_color_data[2][i] = color;
			continue;
		}
		_adjust_color_data[0][i] = hsl2rgb(hsl.h + q[i], hsl.s, hsl.l);
		_adjust_color_data[1][i] = hsl2rgb(hsl.h, hsl.s + q[i]/100, hsl.l);
		_adjust_color_data[2][i] = hsl2rgb(hsl.h, hsl.s, hsl.l + q[i]/100);
	}

}

void ColorPicker::PaintAdjustButtons(){

	GenerateAdjustColors(_old_color);

	RECT rc;
	int base_x = PALETTE_WIDTH + CONTROL_PADDING*2;
	int base_y = CONTROL_PADDING;

	COLORREF color;
	HBRUSH hbrush;
	HDC hdc = ::GetDC(_color_popup);
	
	// frame
	rc.left = base_x;
	rc.top =  base_y;
	rc.right = base_x + ADJUST_BUTTON_WIDTH*3;
	rc.bottom = base_y + ADJUST_BUTTON_HEIGHT*ADJUST_BUTTON_ROW;
	
	// save the area rect for later use
	//::CopyRect(&_rect_adjust, &rc);

	::InflateRect(&rc, 1, 1);

	hbrush = ::CreateSolidBrush(SWATCH_BG_COLOR);
	::FrameRect(hdc, &rc, hbrush);
	::DeleteObject(hbrush);

	// paint current color
	for(int i=0; i<ADJUST_BUTTON_ROW; i++){
		rc.top = base_y + ADJUST_BUTTON_HEIGHT*i;
		rc.bottom = rc.top + ADJUST_BUTTON_HEIGHT;
		rc.left = base_x;
		rc.right = rc.left + ADJUST_BUTTON_WIDTH;
		hbrush = ::CreateSolidBrush(_adjust_color_data[0][i]);
		::FillRect(hdc, &rc, hbrush);
		::DeleteObject(hbrush);
		rc.left = rc.right;
		rc.right = rc.left + ADJUST_BUTTON_WIDTH;
		hbrush = ::CreateSolidBrush(_adjust_color_data[1][i]);
		::FillRect(hdc, &rc, hbrush);
		::DeleteObject(hbrush);
		rc.left = rc.right;
		rc.right = rc.left + ADJUST_BUTTON_WIDTH;
		hbrush = ::CreateSolidBrush(_adjust_color_data[2][i]);
		::FillRect(hdc, &rc, hbrush);
		::DeleteObject(hbrush);
	}

	::ReleaseDC(_color_popup, hdc);

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
	PaintColorPalette();
	PaintColorPreview();

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