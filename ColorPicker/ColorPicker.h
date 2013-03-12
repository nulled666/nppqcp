///////////////////////////////////////////////////////////////////
/*

USAGE:
	create and show:
		ColorPicker* pColorPicker = new ColorPicker();
		pColorPicker->Create(hInstance, hwndParent);
		pColorPicker->Color(RGB(255,0,128));
		pColorPicker->SetParentRect(rc);
		pColorPicker->Show();

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

#define PALETTE_ROW 14
#define PALETTE_COLUMN 24
#define PALETTE_CELL_SIZE 10
#define PALETTE_WIDTH PALETTE_COLUMN * PALETTE_CELL_SIZE
#define PALETTE_HEIGHT PALETTE_ROW * PALETTE_CELL_SIZE
#define RECENT_ZONE_ROW 2
#define RECENT_ZONE_COLUMN 8

#define CONTROL_PADDING 6

#define BUTTON_X CONTROL_PADDING
#define BUTTON_Y PALETTE_HEIGHT + CONTROL_PADDING * 2
#define BUTTON_WIDTH 32
#define BUTTON_HEIGHT 28

#define SWATCH_WIDTH 24
#define SWATCH_HEIGHT BUTTON_HEIGHT

#define POPUP_WIDTH PALETTE_WIDTH + CONTROL_PADDING*2 + 2
#define POPUP_HEIGHT PALETTE_HEIGHT + BUTTON_HEIGHT + CONTROL_PADDING*3 + 2

#define SWATCH_BG_COLOR 0x666666

		
class ColorPicker {

	public:

		ColorPicker(COLORREF color = 0);
		~ColorPicker();

		bool focus_on_show;

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

		void Show();
		void Hide();

		bool IsVisible() const {
			return ( ::IsWindowVisible(_color_popup) ? true : false );
		};
	
    
		// Other stuffs here
		void Color(COLORREF color, bool is_rgb = false);
		COLORREF Color(){
			return _old_color;
		}

		bool SetHexColor(const wchar_t* hex_color);
		void GetHexColor(wchar_t* out);

		void SetRecentColor(const COLORREF* colors);
		void GetRecentColor(COLORREF* &colors);


	protected:

		HINSTANCE _instance;
		HWND _parent_window;
		HWND _color_popup;

		RECT _parent_rc;

	private:
	
		HWND _message_window;

		HCURSOR _pick_cursor;

		bool _old_color_found_in_palette;
		COLORREF _old_color;
		COLORREF _new_color;

		COLORREF _color_palette_data[PALETTE_ROW+1][PALETTE_COLUMN+1];
		COLORREF _recent_color_data[RECENT_ZONE_ROW*RECENT_ZONE_COLUMN];
		
		RECT _rect_palette;
		bool _is_inside_palette;

		int _old_color_row;
		int _old_color_index;

		COLORREF _previous_color;
		int _previous_row;
		int _previous_index;

		bool _is_first_create;
		bool _is_color_chooser_shown;
		
		static void PlaceWindow(const HWND hwnd, const RECT rc);

		// main popup
		static BOOL CALLBACK ColorPopupWinproc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
		BOOL CALLBACK ColorPopupMessageHandle(UINT message, WPARAM wparam, LPARAM lparam);

		void OnInitDialog();
		BOOL OnMouseMove(LPARAM lparam);
		BOOL OnMouseClick(LPARAM lparam);
		
		// palette
		void LoadRecentColorData();
		void PutRecentColor(COLORREF color);
		void FillRecentColorData();

		void GenerateColorPaletteData();

		void DrawColorPalette();
		void DrawColorHoverBox(int row, int index, bool is_hover = true);

		bool PointInRect(const POINT p, const RECT rc);

		void DisplayNewColor(COLORREF color);
		void PaintColorSwatches();

		// screen color picker
		ScreenPicker* _pScreenPicker;
		void StartPickScreenColor();
		void EndPickScreenColor();

		// windows color chooser
		void ShowColorChooser();
		static UINT_PTR CALLBACK ColorChooserWINPROC(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

		double hue(double h, double m1, double m2);
		COLORREF hsl2rgb(int h, double s, double l);
		int round(double number);

};

#endif //COLOR_PICKER_H


