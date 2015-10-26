///////////////////////////////////////////////////////////////////
/*

USAGE:
	create and show:
		ColorPicker* pColorPicker = new ColorPicker();
		pColorPicker->Create(hInstance, hwndParent);
		pColorPicker->SetColor(RGB(255,0,128));
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

#define CONTROL_PADDING 6

#define PALETTE_X CONTROL_PADDING
#define PALETTE_Y CONTROL_PADDING
#define PALETTE_ROW 14
#define PALETTE_COLUMN 24
#define PALETTE_CELL_SIZE 12
#define PALETTE_WIDTH (PALETTE_COLUMN * PALETTE_CELL_SIZE)
#define PALETTE_HEIGHT (PALETTE_ROW * PALETTE_CELL_SIZE)

#define RECENT_ZONE_ROW 2
#define RECENT_ZONE_COLUMN 8


#define ADJUST_ZONE_X (PALETTE_X + PALETTE_WIDTH + CONTROL_PADDING)
#define ADJUST_ZONE_Y CONTROL_PADDING
#define ADJUST_ZONE_ROW 9
#define ADJUST_ZONE_CELL_WIDTH 12
#define ADJUST_ZONE_CELL_HEIGHT 12
#define ADJUST_ZONE_WIDTH (ADJUST_ZONE_CELL_WIDTH*3+2)
#define ADJUST_ZONE_HEIGHT (ADJUST_ZONE_CELL_HEIGHT*ADJUST_ZONE_ROW)

#define SWATCH_X ADJUST_ZONE_X
#define SWATCH_Y (ADJUST_ZONE_Y + ADJUST_ZONE_HEIGHT + CONTROL_PADDING)
#define SWATCH_WIDTH (ADJUST_ZONE_WIDTH/2)
#define SWATCH_HEIGHT (PALETTE_HEIGHT - ADJUST_ZONE_HEIGHT - CONTROL_PADDING)

#define BUTTON_X CONTROL_PADDING
#define BUTTON_Y (PALETTE_HEIGHT + CONTROL_PADDING*2 + 1)
#define BUTTON_WIDTH 32
#define BUTTON_HEIGHT 32

#define TEXT_X (BUTTON_X + BUTTON_WIDTH*2 + CONTROL_PADDING*2)
#define TEXT_Y (BUTTON_Y + (BUTTON_HEIGHT-12)/2)
#define TEXT_WIDTH (POPUP_WIDTH-TEXT_X-CONTROL_PADDING)
#define TEXT_HEIGHT 16

#define POPUP_WIDTH (PALETTE_WIDTH + ADJUST_ZONE_WIDTH + CONTROL_PADDING*3 +  2)
#define POPUP_HEIGHT (PALETTE_HEIGHT + BUTTON_HEIGHT + CONTROL_PADDING*3 + 2)

#define CONTROL_BORDER_COLOR 0x666666




class ColorPicker {

	public:

		ColorPicker(COLORREF color = 0);
		~ColorPicker();

		struct HSLCOLOR{
			double h;
			double s;
			double l;
		};

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
		void SetColor(COLORREF color);
		COLORREF GetColor(){
			return _old_color;
		}

		bool SetHexColor(const char* hex_color);
		void GetHexColor(char* out_hex, int buffer_size);

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
		bool _show_picker_cursor;

		bool _old_color_found_in_palette;
		COLORREF _old_color;
		COLORREF _new_color;

		COLORREF _color_palette_data[PALETTE_ROW+1][PALETTE_COLUMN+1];
		COLORREF _recent_color_data[RECENT_ZONE_ROW*RECENT_ZONE_COLUMN];

		RECT _rect_palette;

		int _old_color_row;
		int _old_color_index;

		COLORREF _previous_color;
		int _previous_row;
		int _previous_index;

		RECT _rect_adjust_buttons;

		COLORREF _adjust_color_data[3][ADJUST_ZONE_ROW];
		COLORREF _adjust_color;
		double _adjust_preserved_hue;
		double _adjust_preserved_saturation;
		int _adjust_center_row;
		int _adjust_row;
		int _adjust_index;

		bool _is_first_create;
		bool _is_color_chooser_shown;
		
		static void PlaceWindow(const HWND hwnd, const RECT rc);

		// main popup
		static BOOL CALLBACK ColorPopupWinproc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
		BOOL CALLBACK ColorPopupMessageHandle(UINT message, WPARAM wparam, LPARAM lparam);

		void OnInitDialog();
		BOOL OnMouseMove(LPARAM lparam);
		BOOL OnMouseClick(LPARAM lparam, bool is_right_button);

		bool PointInRect(const POINT p, const RECT rc);
		void PaintAll();

		// color preview
		void PaintColorSwatch();
		void DisplayNewColor(COLORREF color);
		
		// palette
		void GenerateColorPaletteData();
		void LoadRecentColorData();
		void SaveToRecentColor(COLORREF color);
		void FillRecentColorData();

		void PaintColorPalette();
		void DrawColorHoverBox(int row, int index, bool is_hover = true);
		void PaletteMouseMove(const POINT p);
		void PaletteMouseClick(const POINT p, bool is_right_button);

		void GenerateAdjustColors(COLORREF color);
		void PaintAdjustZone();
		void DrawAdjustZoneHoverBox(int row, int index, bool is_hover = true);
		void AdjustZoneMouseMove(const POINT p);
		void AdjustZoneMouseClick(const POINT p, bool is_right_button);

		// screen color picker
		ScreenPicker* _pScreenPicker;
		void StartPickScreenColor();
		void EndPickScreenColor();

		// windows color chooser
		void ShowColorChooser();
		static UINT_PTR CALLBACK ColorChooserWINPROC(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

		// helper functions
		HSLCOLOR rgb2hsl(const COLORREF rgb);
		COLORREF hsl2rgb(const HSLCOLOR hsl);
		COLORREF hsl2rgb(const double h, const double s, const double l);
		double hue(double h, double m1, double m2);

		int round(double number);

};

#endif //COLOR_PICKER_H


