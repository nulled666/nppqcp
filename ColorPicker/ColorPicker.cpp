

#include "ColorPicker.h"
#include "ScreenPicker.h"

#include <stdio.h>
#include <iostream>
#include <math.h>

#include <windowsx.h>


#define BG_COLOR 0xEEEEEE
#define POPUP_WIDTH 266
#define POPUP_HEIGHT 220

#define SWATCH_BG_COLOR 0x666666

ColorPicker::ColorPicker() {
	
	_instance = NULL;
	_parent_window = NULL;
	_color_popup = NULL;
	
	_message_window = NULL;


	_pick_cursor = NULL;

	_current_color_found_in_palette = false;
	_current_color = 0x000000;
	_new_color = 0x000000;

	_color_palette_data[14][21] = NULL;
	_recent_color_data[10] = NULL;
		
	_color_palette = NULL;
	_default_color_palette_winproc = NULL;

	_color_swatch_current = NULL;
	_color_swatch_new = NULL;

	_hbrush_popup_bg = NULL;
	_hbrush_swatch_current = NULL;
	_hbrush_swatch_new = NULL;
	_hbrush_swatch_bg = NULL;

	_is_color_chooser_shown = false;

	_pScreenPicker = NULL;

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

	_color_popup = ::CreateDialogParam(_instance, MAKEINTRESOURCE(IDD_COLOR_PICKER_POPUP), _parent_window, (DLGPROC)ColorPopupWINPROC, (LPARAM)this);
	
	if (message_window == NULL) {
		_message_window = message_window;
	} else {
		_message_window = message_window;
	}

	if (!_color_popup) {
		throw std::runtime_error("ColorPicker::Create() : CreateDialogParam() function returns null");
	}
	
}

void ColorPicker::Destroy() {

	::RemoveProp(_color_palette, L"parent");

	::DestroyWindow(_color_popup);

	::DeleteObject(_hbrush_popup_bg);
	::DeleteObject(_hbrush_swatch_current);
	::DeleteObject(_hbrush_swatch_new);
	::DeleteObject(_hbrush_swatch_bg);

}

void ColorPicker::Color(COLORREF color, bool is_rgb) {
	
	if (is_rgb) {
		color = RGB(GetBValue(color), GetGValue(color), GetRValue(color));
	}
	
	_current_color = color;
	_current_color_found_in_palette = false;

	DisplayNewColor(color);

	// remove hover box
	::SendDlgItemMessage(_color_popup, IDC_COLOR_PALETTE, LB_SETCURSEL, -1, NULL);

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

BOOL CALLBACK ColorPicker::ColorPopupWINPROC(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {

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
		case WM_CTLCOLORDLG:
		case WM_CTLCOLOR:
		{
			if (_hbrush_popup_bg == NULL) {
				_hbrush_popup_bg = CreateSolidBrush(BG_COLOR);
			}
            return (LRESULT) _hbrush_popup_bg;
		}
		case WM_CTLCOLORSTATIC:
		{
			return OnCtlColorStatic(lparam);
		}
		case WM_DRAWITEM:
		{
			return OnDrawItem(lparam);
		}
		case WM_SCREEN_HOVER_COLOR:
		{
			DisplayNewColor((COLORREF)lparam);
			return TRUE;
		}
		case WM_SCREEN_PICK_COLOR:
		{
			COLORREF color = (COLORREF)lparam;
			PutRecentColor(color);
			::SetActiveWindow(_color_popup);
			::SendMessage(_message_window, WM_PICKUP_COLOR, color, 0);
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
                case IDC_COLOR_PALETTE: {
			        if (HIWORD(wparam) == LBN_SELCHANGE) {
                        OnSelectColor(lparam);
					    return TRUE;
		            }
					return FALSE;
				}
                default: {
                    return FALSE;
				}
            }
		}
		case WM_ACTIVATE:
		{
			if (LOWORD(wparam) == WA_INACTIVE) {
				if (!_is_color_chooser_shown) {
					::SendMessage(_message_window, WM_PICKUP_CANCEL, 0, 0);
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

	_color_palette = ::GetDlgItem(_color_popup, IDC_COLOR_PALETTE);
	_color_swatch_current = ::GetDlgItem(_color_popup, IDC_OLD_COLOR);
	_color_swatch_new = ::GetDlgItem(_color_popup, IDC_NEW_COLOR);

	// dialog control ui stuff
	_pick_cursor = ::LoadCursor(_instance, MAKEINTRESOURCE(IDI_CURSOR));
	HICON hIcon = (HICON)::LoadImage(_instance, MAKEINTRESOURCE(IDI_PICKER), IMAGE_ICON, 16, 16, 0);

	::SetWindowPos(_color_palette, NULL, 6, 6, 253, 169, SWP_NOACTIVATE);

	HWND ctrl = ::GetDlgItem(_color_popup, ID_PICK);
	::SetWindowPos(ctrl, NULL, 6, 184, 32, 28, SWP_NOZORDER);
	::SendDlgItemMessage(_color_popup, ID_PICK, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcon);

	ctrl = ::GetDlgItem(_color_popup, ID_MORE);
	::SetWindowPos(ctrl, NULL, 43, 184, 32, 28, SWP_NOZORDER);

	ctrl = ::GetDlgItem(_color_popup, IDC_COLOR_BG);
	::SetWindowPos(ctrl, NULL, 208, 186, 50, 24, SWP_NOZORDER);

	::SetWindowPos(_color_swatch_current, NULL, 233, 187, 24, 22, SWP_NOZORDER);
	::SetWindowPos(_color_swatch_new, NULL, 209, 187, 24, 22, SWP_NOZORDER);

	ctrl = ::GetDlgItem(_color_popup, IDC_COLOR_TEXT);
	::SetWindowPos(ctrl, NULL, 100, 191, 80, 20, SWP_NOZORDER);

	// generate palette color array
	GenerateColorPaletteData();
	LoadRecentColorData();
	FillRecentColorData();
			
	// fill palette
	int id = 0;
	for (int index = 0; index < 21; index++) {
		for (int row = 0; row < 14; row++) {
			::SendDlgItemMessage(_color_popup, IDC_COLOR_PALETTE, LB_ADDSTRING, id, (LPARAM)"");
			::SendDlgItemMessage(_color_popup, IDC_COLOR_PALETTE, LB_SETITEMDATA , id, (LPARAM)_color_palette_data[row][index]);
			id++;
		}
	}
	
	// Subclassing the color palette control
	::SetProp(_color_palette, L"parent", (HANDLE)this);
	_default_color_palette_winproc = (WNDPROC) ::SetWindowLongPtr(_color_palette, GWL_WNDPROC, (LONG)ColorPaletteWINPROC);			

}


// WM_ONCTLCOLORSTATIC
LRESULT ColorPicker::OnCtlColorStatic(LPARAM lparam) {

	DWORD ctrl_id = ::GetDlgCtrlID((HWND)lparam);

	switch (ctrl_id) {
		case IDC_OLD_COLOR:
		{
			if(_hbrush_swatch_current != NULL) {
				::DeleteObject(_hbrush_swatch_current);
			}
			_hbrush_swatch_current = CreateSolidBrush(_current_color);
			return (LRESULT)_hbrush_swatch_current;
		}
		case IDC_NEW_COLOR:
		{
			if(_hbrush_swatch_new != NULL) {
				::DeleteObject(_hbrush_swatch_new);
			}
			_hbrush_swatch_new = CreateSolidBrush(_new_color);
			return (LRESULT)_hbrush_swatch_new;
		}
		case IDC_COLOR_BG:
		{
			if (_hbrush_swatch_bg == NULL) {
				_hbrush_swatch_bg = CreateSolidBrush(SWATCH_BG_COLOR);
			}
			return (LRESULT)_hbrush_swatch_bg;
		}
		default:
		{
			return FALSE;
		}
	}

}

// WM_DRAWENTIRE
BOOL ColorPicker::OnDrawItem(LPARAM lParam){
	
	DRAWITEMSTRUCT *pdis = (DRAWITEMSTRUCT*)lParam;
	HDC  hdc = pdis->hDC;
	
	// Transparent bg
	SetBkMode(hdc, TRANSPARENT);
	
	// NULL object
	if (pdis->itemID == UINT(-1)) {
		return FALSE; 
	}
			
	RECT rc = pdis->rcItem;
	COLORREF cr = (COLORREF)pdis->itemData;
	HBRUSH hbrush;
	
	if (pdis->CtlID == IDC_COLOR_PALETTE) {
		
		switch (pdis->itemAction) {
			case ODA_DRAWENTIRE:
			{
				rc = pdis->rcItem;
				rc.right++;
				rc.bottom++;
				hbrush = ::CreateSolidBrush((COLORREF)cr);
				::FillRect(hdc, &rc, hbrush);
				::DeleteObject(hbrush);
				hbrush = ::CreateSolidBrush(RGB(0,0,0));
				::FrameRect(hdc, &rc, hbrush);
				::DeleteObject(hbrush);
				// *** FALL THROUGH ***
			}
			case ODA_SELECT:
			{
				rc = pdis->rcItem;
				rc.right++;
				rc.bottom++;
				if (pdis->itemState & ODS_SELECTED)	{
					// hovered cell - draw a invert inner box
					::InflateRect(&rc, -1, -1);
					COLORREF color = cr ^ 0xffffff;
					hbrush = ::CreateSolidBrush( color );
					::FrameRect(hdc, &rc, hbrush);
					::DeleteObject(hbrush);
				} else {
					// normal cell - redraw the inner box
					::InflateRect(&rc, -1, -1);
					if (!_current_color_found_in_palette && cr == _current_color) {
						hbrush = ::CreateSolidBrush(cr ^ 0xffffff);
						_current_color_found_in_palette = true;
					} else {
						hbrush = ::CreateSolidBrush((COLORREF)cr);
					}
					::FrameRect(hdc, &rc, hbrush);
					::DeleteObject(hbrush);
				}
				break;
			}
			default:
			{
				// nothing
			}
		}

	}

	return TRUE;

}

// WM_COMMAND -> WM_COMMAND -> LBN_SELCHANGE
void ColorPicker::OnSelectColor(LPARAM lparam){

	int index = ::SendMessage((HWND)lparam, LB_GETCURSEL, 0L, 0L);
    COLORREF color = ::SendMessage((HWND)lparam, LB_GETITEMDATA, index, 0L);
	
	_current_color = color;

	PutRecentColor(color);

	::SendMessage(_message_window, WM_PICKUP_COLOR, _current_color, 0);

}

// WM_COMMAND -> ID_PICK
// track mouse
void ColorPicker::StartPickScreenColor(){
	
	if (!_pScreenPicker) {
		_pScreenPicker = new ScreenPicker();
		_pScreenPicker->Create(_instance, _color_popup);
	}

	_pScreenPicker->Start();

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
	cc.rgbResult = _current_color;
	cc.Flags = CC_FULLOPEN | CC_RGBINIT | CC_ENABLEHOOK;
	cc.lpfnHook = (LPCCHOOKPROC)ColorChooserWINPROC;

	Show(false);
	_is_color_chooser_shown = true;

	if (ChooseColor(&cc)==TRUE) {
		PutRecentColor(cc.rgbResult);
		::SendMessage(_message_window, WM_PICKUP_COLOR, cc.rgbResult, 0);
	} else {
		::SendMessage(_message_window, WM_PICKUP_CANCEL, 0, 0);
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

	// update swatch color - lazy solution
	::SetDlgItemText(_color_popup, IDC_NEW_COLOR, L"");

	// transform order for hex display
	color = RGB(GetBValue(color), GetGValue(color), GetRValue(color));
	
	wchar_t hex[8];
	wsprintf(hex, L"#%06X", color);
	::SetDlgItemText(_color_popup, IDC_COLOR_TEXT, hex);

}


///////////////////////////////////////////////////////////////////
// COLOR PALETTE
///////////////////////////////////////////////////////////////////
 
//Subclassing Procedure for palette
LRESULT CALLBACK ColorPicker::ColorPaletteWINPROC(HWND hWnd, UINT message, WPARAM wParam,LPARAM lParam)
{
	ColorPicker* hPopup = (ColorPicker*) ::GetProp(hWnd, L"parent");

	return hPopup->ColorPaletteMessageHandle(hWnd, message, wParam, lParam);
}

BOOL ColorPicker::ColorPaletteMessageHandle(HWND hWnd, UINT message, WPARAM wParam,LPARAM lParam)
{
    switch(message)
    {
		case WM_SETCURSOR:
			::SetCursor(_pick_cursor);
			return TRUE;

		case WM_MOUSEMOVE:
			TRACKMOUSEEVENT tme;
			tme.cbSize = sizeof(tme);
			tme.dwFlags = TME_LEAVE;
			tme.hwndTrack = hWnd;
			TrackMouseEvent(&tme);

			OnColorPaletteHover(lParam);
			return TRUE;
         
		case WM_MOUSELEAVE:
			DisplayNewColor(_current_color);
			return TRUE;

		default:
			return CallWindowProc(_default_color_palette_winproc, hWnd, message, wParam, lParam);
    }
}


void ColorPicker::OnColorPaletteHover(LPARAM lParam){
	
	LRESULT index = ::SendDlgItemMessage(_color_popup, IDC_COLOR_PALETTE, LB_ITEMFROMPOINT, 0, lParam);
		
	if(index < 0 || index > (21*14-1) ) return;

	::SendDlgItemMessage(_color_popup, IDC_COLOR_PALETTE, LB_SETCURSEL, (WPARAM)LOWORD(index), NULL);

	COLORREF color = (COLORREF)::SendDlgItemMessage(_color_popup, IDC_COLOR_PALETTE, LB_GETITEMDATA , index, NULL);
			
	DisplayNewColor(color);

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
		int id = 14*i;
		::SendDlgItemMessage(_color_popup, IDC_COLOR_PALETTE, LB_SETITEMDATA , id, (LPARAM)_color_palette_data[0][i]);
		::SendDlgItemMessage(_color_popup, IDC_COLOR_PALETTE, LB_SETITEMDATA , id+1, (LPARAM)_color_palette_data[1][i]);
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