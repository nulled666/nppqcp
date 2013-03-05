///////////////////////////////////////////////////////////////////
/*

USAGE:
	create and show:
		ColorPicker* pColorPicker = new ColorPicker();
		pColorPicker->Create(hInstance, hwndParent);
		pColorPicker->Color(RGB(255,0,128));
		pColorPicker->SetParentRect(rc);
		pColorPicker->Show(true);

	messages:
		WM_QCP_PICK
			User picked a color
			LPARAM is the color in COLORREF format
		WM_QCP_CANCEL
			User cancelled the operation


*/
///////////////////////////////////////////////////////////////////


#ifndef COLOR_PICKER_H
#define COLOR_PICKER_H

#include "ScreenPicker.h"
#include <windows.h>

#ifndef COLOR_PICKER_RESOURCE_H
#include "ColorPicker.res.h"
#endif //COLOR_PICKER_RESOURCE_H


class ColorPicker {

	public:

		ColorPicker(COLORREF color = 0);
		~ColorPicker();

		void Create(HINSTANCE instance, HWND parent, HWND message_window = NULL);
		void Destroy();
  
		bool IsCreated() const {
			return (_color_popup != NULL);
		};
	
		HWND GetWindow() const {
			return _color_popup;
		};
		HWND GetParentWindow() const {
			return _parent_window;
		};
		HINSTANCE GetInstance() const {
			return _instance;
		};
	
		// specify a message window for return value if needed
		void SetMessageWindow(HWND hwnd) {
			_message_window = hwnd;
		}

		void SetParentRect(RECT rc);

		void Show(bool show = true) const {
			::ShowWindow(_color_popup, show ? SW_SHOW : SW_HIDE);
		};

		bool IsVisible() const {
			return ( ::IsWindowVisible(_color_popup) ? true : false );
		};
	
    
		// Other stuffs here
		void Color(COLORREF color, bool is_rgb = false);
		COLORREF Color(){
			return _current_color;
		}

		bool SetHexColor(const wchar_t* hex_color);
		void GetHexColor(wchar_t* out);

		void SetRecentColor(const COLORREF* colors);
		void GetRecentColor(COLORREF* &colors);


	protected:

		HINSTANCE _instance;
		HWND _parent_window;
		HWND _color_popup;


	private:
	
		HWND _message_window;

		HCURSOR _pick_cursor;

		bool _current_color_found_in_palette;
		COLORREF _current_color;
		COLORREF _new_color;

		COLORREF _color_palette_data[14][21];
		COLORREF _recent_color_data[10];
		
		HWND _color_palette;
		WNDPROC _default_color_palette_winproc;

		HWND _color_swatch_current;
		HWND _color_swatch_new;

		HBRUSH _hbrush_popup_bg;
		HBRUSH _hbrush_swatch_current;
		HBRUSH _hbrush_swatch_new;
		HBRUSH _hbrush_swatch_bg;

		bool _is_color_chooser_shown;
		
		// main popup
		static BOOL CALLBACK ColorPopupWINPROC(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
		BOOL CALLBACK ColorPopupMessageHandle(UINT message, WPARAM wparam, LPARAM lparam);

		void OnInitDialog();
		LRESULT OnCtlColorStatic(LPARAM lparam);
		BOOL OnDrawItem(LPARAM lparam);
		void OnSelectColor(LPARAM lparam);

		// palette
		static LRESULT CALLBACK ColorPaletteWINPROC(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
		BOOL CALLBACK ColorPaletteMessageHandle(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

		void LoadRecentColorData();
		void PutRecentColor(COLORREF color);
		void FillRecentColorData();
		void GenerateColorPaletteData();
		void OnColorPaletteHover(LPARAM lparam);

		// screen color picker
		ScreenPicker* _pScreenPicker;
		void StartPickScreenColor();
		void EndPickScreenColor();

		// windows color chooser
		void ShowColorChooser();
		static UINT_PTR CALLBACK ColorChooserWINPROC(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

		void DisplayNewColor(COLORREF color);

		int round(double number);

};

#endif //COLOR_PICKER_H


