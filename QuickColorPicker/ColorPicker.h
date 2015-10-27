///////////////////////////////////////////////////////////////////
/*

USAGE:
	create and show:

		#include "QuickColorPicker\ColorPicker.h"
		#include "QuickColorPicker\ColorPicker.res.h"

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

namespace QuickColorPicker {
	

	struct RGBAColor {
		inline RGBAColor() {}
		inline RGBAColor(byte r, byte g, byte b, float a)
			: r(r), g(g), b(b), a(a) {}

		unsigned char r = 0, g = 0, b = 0;
		float a = 1.0f;

		inline RGBAColor(COLORREF color) {
			r = GetRValue(color);
			g = GetGValue(color);
			b = GetBValue(color);
		}

		inline bool operator==(RGBAColor rgba) {
			if (rgba.r == r && rgba.g == g && rgba.b == b && rgba.a == a)
				return true;
			else
				return false;
		}

		inline operator COLORREF() {
			return RGB(r, g, b);
		}

	};


	struct HSLAColor {
		inline HSLAColor() {}
		inline HSLAColor(double h, double s, double l, float a = 1.0f)
			: h(h), s(s), l(l), a(a) {}
		double h = 0, s = 0, l = 0;
		float a = 1.0f;
	};

	

	class ColorPicker {

	public:

		ColorPicker();
		~ColorPicker();

		bool focus_on_show;

		void Create(HINSTANCE instance, HWND parent, HWND message_window = NULL);

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
			return (::IsWindowVisible(_color_popup) ? true : false);
		};


		// Other stuffs here
		void SetColor(RGBAColor color);
		RGBAColor GetColor() {
			return _old_color;
		}

		bool SetHexColor(const char* hex_color);
		void GetHexColor(char* out_hex, int buffer_size);

		void SetRecentColor(const RGBAColor* colors);
		void GetRecentColor(RGBAColor* &colors);


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
		RGBAColor _old_color;
		RGBAColor _new_color;

		RGBAColor _color_palette_data[PALETTE_ROW + 1][PALETTE_COLUMN + 1];
		RGBAColor _recent_color_data[RECENT_ZONE_ROW*RECENT_ZONE_COLUMN];

		RECT _rect_palette;

		int _old_color_row;
		int _old_color_index;

		RGBAColor _previous_color;
		int _previous_row;
		int _previous_index;

		RECT _rect_adjust_buttons;

		RGBAColor _adjust_color_data[3][ADJUST_ZONE_ROW];
		RGBAColor _adjust_color;
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
		void DisplayNewColor(RGBAColor color);

		// palette
		void GenerateColorPaletteData();
		void LoadRecentColorData();
		void SaveToRecentColor(RGBAColor color);
		void FillRecentColorData();

		void PaintColorPalette();
		void DrawColorHoverBox(int row, int index, bool is_hover = true);
		void PaletteMouseMove(const POINT p);
		void PaletteMouseClick(const POINT p, bool is_right_button);

		void GenerateAdjustColors(RGBAColor color);
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
		HSLAColor rgb2hsl(const RGBAColor rgb);
		RGBAColor hsl2rgb(const HSLAColor hsl);
		RGBAColor hsl2rgb(const double h, const double s, const double l, const float a);
		double calc_color(double c, double t1, double t2);

		int round(double number);

	};

}

#endif //COLOR_PICKER_H


