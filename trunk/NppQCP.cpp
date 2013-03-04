// Quick Color Picker plugin for Notepad++

#include "NppQCP.h"

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <shlwapi.h>

#include "ColorPicker\ColorPicker.h"
#include "ColorPicker\ColorPicker.res.h"


const wchar_t _ini_section[] = L"nppqcp";
const wchar_t _ini_key[] = L"enabled";
const wchar_t _ini_file[] = L"nppqcp.ini";


//
// The data of Notepad++ that you can use in your plugin commands
// extern variables - don't change the name
NppData nppData;
FuncItem funcItem[_command_count];
bool doCloseTag;


TCHAR _ini_file_path[MAX_PATH];
bool _enable_qcp = false;

bool _is_color_picker_shown = false;


ColorPicker* _pColorPicker;
HINSTANCE _instance;
HWND _message_window;


//
// Initialize your plugin data here
// It will be called while plugin loading   
void PluginInit(HANDLE module) {

	// save for internal dialogs
	_instance = (HINSTANCE)module;

	CreateMessageWindow();

}

//
// Here you can do the clean up, save the parameters (if any) for the next session
//
void PluginCleanUp() {

	DestroyMessageWindow();

}



////////////////////////////////////////
// Access config file
////////////////////////////////////////
void LoadConfig(){

	// get path of plugin configuration
	::SendMessage(nppData._nppHandle, NPPM_GETPLUGINSCONFIGDIR, MAX_PATH, (LPARAM)_ini_file_path);

	if (PathFileExists(_ini_file_path) == FALSE) {
		::CreateDirectory(_ini_file_path, NULL);
	}

	PathAppend(_ini_file_path, _ini_file);

	// read in config
	int enabled = ::GetPrivateProfileInt(_ini_section, _ini_key, 1, _ini_file_path);
	_enable_qcp = ( enabled == 1);

}

void SaveConfig(){

	::WritePrivateProfileString(
		_ini_section, _ini_key,
		_enable_qcp ? L"1" : L"0",
		_ini_file_path
	);

}


////////////////////////////////////////
// Menu Command
////////////////////////////////////////
void InitCommandMenu() {

    // setCommand(int index,                      // zero based number to indicate the order of command
    //            wchar_t *commandName,             // the command name that you want to see in plugin menu
    //            PFUNCPLUGINCMD functionPointer, // the symbol of function (function pointer) associated with this command. The body should be defined below. See Step 4.
    //            ShortcutKey *shortcut,          // optional. Define a shortcut to trigger this command
    //            bool checkOnInit                // optional. Make this menu item be checked visually
    //            );

	setCommand(0, L"Enable Quick Color Picker", ToggleQCP, NULL, _enable_qcp);

	setCommand(1, L"---", NULL, NULL, false);

	wchar_t text[200] = L"Visit Website ";
	wcscat(text, L" (Version ");
	wcscat(text, NPP_PLUGIN_VER);
	wcscat(text, L")");
	setCommand(2, text, VisitWebsite, NULL, FALSE);

}

bool setCommand(size_t index, wchar_t *cmdName, PFUNCPLUGINCMD pFunc, ShortcutKey *sk, bool checkOnInit) {

    if (index >= _command_count)
        return false;

    if (!pFunc)
        return false;

    wcscpy(funcItem[index]._itemName, cmdName);
    funcItem[index]._pFunc = pFunc;
    funcItem[index]._init2Check = checkOnInit;
    funcItem[index]._pShKey = sk;

    return true;
}

void commandMenuCleanUp() {

}


///////////////////////////////////////////////////////////
// MENU COMMANDS
///////////////////////////////////////////////////////////

// toggle QCP plugins
void ToggleQCP() {

	_enable_qcp = !_enable_qcp;
	
	if (_enable_qcp) {
		HighlightColorCode();
	} else {
		RemoveColorHighlight();
		HideColorPicker();
	}

	::CheckMenuItem(::GetMenu(nppData._nppHandle), funcItem[0]._cmdID, MF_BYCOMMAND | (_enable_qcp ? MF_CHECKED : MF_UNCHECKED));

}

// visit nppqcp website
void VisitWebsite() {

	wchar_t url[200] = L"http://code.google.com/p/nppqcp/";
	::ShellExecute(NULL, L"open", url, NULL, NULL, SW_SHOWNORMAL);

}


///////////////////////////////////////////////////////////
//  MESSAGE WINDOW
///////////////////////////////////////////////////////////

// create a background window for message communication
void CreateMessageWindow() {
	
	static wchar_t szWindowClass[] = L"npp_qcp_msgwin";
	
	WNDCLASSEX wc    = {0};
	wc.cbSize        = sizeof(wc);
	wc.lpfnWndProc   = MessageWindowWINPROC;
	wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = _instance;
	wc.lpszClassName = szWindowClass;

	if (!RegisterClassEx(&wc)) {
		throw std::runtime_error("NppQCP: RegisterClassEx() failed");
	}

	_message_window = CreateWindowEx(0, szWindowClass, szWindowClass, WS_POPUP,	0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL);

	if (!_message_window) {
		throw std::runtime_error("NppQCP: CreateWindowEx() function returns null");
	}

}

void DestroyMessageWindow() {

	::DestroyWindow(_message_window);
	::UnregisterClass(L"npp_qcp_msgwin", _instance);

}

// message proccessor
LRESULT CALLBACK MessageWindowWINPROC(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {

	switch (message) {
		case WM_PICKUP_COLOR: {
			WriteColorCodeToEditor((COLORREF)wparam);
			break;
		}
		default: {
			return TRUE;
		}
	}

	return TRUE;

}


// Get current scintilla comtrol hwnd
HWND GetScintilla() {

    int which = -1;
    ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&which);
    if (which == -1)
        return NULL;

    HWND h_scintilla = (which == 0) ? nppData._scintillaMainHandle : nppData._scintillaSecondHandle;
	
	return h_scintilla;

}



////////////////////////////////////////////////////////////////////////////////
// COLOR PALETTE POPUP
////////////////////////////////////////////////////////////////////////////////

void CreateColorPicker(){

	_pColorPicker = new ColorPicker();
	_pColorPicker->Create(_instance, nppData._nppHandle, _message_window);

	LoadRecentColor();

	::SetActiveWindow(nppData._nppHandle);

}

bool ShowColorPicker(){

	if (!_enable_qcp)
		return false;

	HWND h_scintilla = GetScintilla();

	// detect whether selected content is a color code
	int selection_start = ::SendMessage(h_scintilla, SCI_GETSELECTIONSTART, 0, 0);
	int selection_end = ::SendMessage(h_scintilla, SCI_GETSELECTIONEND, 0, 0);
	int len = selection_end - selection_start;

	// break - wrong length
	if (len != 3 && len != 6)
		return false;

	char mark = (char)::SendMessage(h_scintilla, SCI_GETCHARAT, selection_start - 1, 0);

	// break - no # mark
	if (mark == 0 || mark != '#')
		return false;

	// passed -

	// create the color picker if not created
	if (!_pColorPicker)
		CreateColorPicker();

	// get hex text - scintilla only accept char
	char hex_str[10];
    ::SendMessage(h_scintilla, SCI_GETSELTEXT , 0, (LPARAM)&hex_str);

	wchar_t hex_color[10];

	::MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, hex_str, -1, hex_color, sizeof(hex_color));

	// put current color to picker
	if (!_pColorPicker->SetHexColor(hex_color)) {
		// not a valid hex color string
		return false;
	}
	
	// prepare coordinates
	POINT p;
	p.x = ::SendMessage(h_scintilla, SCI_POINTXFROMPOSITION , 0, selection_start);
	p.y = ::SendMessage(h_scintilla, SCI_POINTYFROMPOSITION , 0, selection_start);
	::ClientToScreen(h_scintilla, &p);
	 // all line height in scintilla is the same
	int line_height = ::SendMessage(h_scintilla, SCI_TEXTHEIGHT , 0, 1);

	RECT rc;
	rc.top = p.y;
	rc.right = p.x; // not used anyway
	rc.bottom = p.y + line_height;
	rc.left = p.x;
	
	_pColorPicker->SetParentRect(rc);

	// set and show
	_pColorPicker->Show(true);

	// set the focus back to editor - don't break the keyboard action chain
	::SetActiveWindow(nppData._nppHandle);

	return true;

}


// hide the palette
void HideColorPicker() {
	
	if (!_pColorPicker || !_pColorPicker->IsVisible())
		return;

	_pColorPicker->Show(false);

}


void ToggleColorPicker(){

	if (!_pColorPicker)
		CreateColorPicker();

	if (!_pColorPicker->IsVisible()) {
		ShowColorPicker();
	} else {
		HideColorPicker();
	}

}


void WriteColorCodeToEditor(COLORREF color) {

	// convert to rgb color
	color = RGB(GetBValue(color),GetGValue(color),GetRValue(color));

	// Replace with new hex color string
	// SCI_REPLACESEL only accept char*
	char buff[10];
	sprintf(buff, "%0*X", 6, color);
		
	HWND current_scintilla = GetScintilla();
	::SendMessage(current_scintilla, SCI_REPLACESEL, NULL, (LPARAM)(char*)buff);

	::SetActiveWindow(current_scintilla);

	SaveRecentColor();

}

// load & save recent used colors
void LoadRecentColor(){

	if (!_pColorPicker)
		return;

	COLORREF colors[10];
	int color;
	wchar_t key[20];

	for (int i=0; i<10; i++) {
		wsprintf(key, L"recent%d", i);
		colors[i] = ::GetPrivateProfileInt(_ini_section, key, 0, _ini_file_path);		
	}

	_pColorPicker->SetRecentColor(colors);
}

void SaveRecentColor(){
	
	if(!_pColorPicker)
		return;

	COLORREF colors[10];
	COLORREF* p_colors = colors;
	_pColorPicker->GetRecentColor(p_colors);

	wchar_t color[10];
	wchar_t key[20];

	for (int i=0; i<10; i++) {
		wsprintf(color, L"%d", colors[i]);
		wsprintf(key, L"recent%d", i);
		::WritePrivateProfileString(_ini_section, key, color, _ini_file_path);
	}

}


bool is_hex(char* str)
{
	char copy_of_param [64];
	
	return (
		strtok ( strcpy(copy_of_param, str), "0123456789ABCDEFabcdef" ) == NULL
	);
}

bool pad_hex(char* out, const char* in) {

	if (strlen(in) != 3){
		return false;
	}

	// the stupid and fast way
	out[0] = in[0];
    out[1] = in[0];
    out[2] = in[1];
    out[3] = in[1];
    out[4] = in[2];
    out[5] = in[2];
	out[6] = '\0';

	return true;

}

COLORREF revert_color_order(COLORREF color) {
	return RGB(GetBValue(color),GetGValue(color),GetRValue(color));
}


COLORREF hex2color(char* str) {
	COLORREF color = (COLORREF)strtol(str, NULL, 16);
	return revert_color_order(color);
}



////////////////////////////////////////////////////////////////////////////////
// highlight hex color
////////////////////////////////////////////////////////////////////////////////
void HighlightColorCode() {

	if(!_enable_qcp)
		return;

	int lang = -100;
    ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTLANGTYPE, 0, (LPARAM)&lang);

    if (lang != L_CSS) {
        return;
    }
    
    RemoveColorHighlight();

    HWND h_scintilla = GetScintilla();

    int save_caret_start = ::SendMessage(h_scintilla, SCI_GETSELECTIONSTART, 0, 0);
    int save_caret_end = ::SendMessage(h_scintilla, SCI_GETSELECTIONEND, 0, 0);

    //SCI_GETFIRSTVISIBLELINE first line is 0
    int first_visible_line = ::SendMessage(h_scintilla, SCI_GETFIRSTVISIBLELINE, 0, 0);
    
    int first_line_position = ::SendMessage(h_scintilla, SCI_POSITIONFROMLINE, first_visible_line, 0);
    ::SendMessage(h_scintilla, SCI_SETANCHOR, first_line_position, 0);
    ::SendMessage(h_scintilla, SCI_SEARCHANCHOR, 0, 0);


    char found[9999];
    strcpy(found, "");

    int i = 1;
    int indicator;

    int set_colors = 0;


    while (i <= 20)
    {
        ::SendMessage(h_scintilla, SCI_SEARCHANCHOR, 0, 0);
		::SendMessage(h_scintilla, SCI_SEARCHNEXT, SCFIND_REGEXP, (LPARAM)"#[0-9AaBbCcDdEeFf]{3,6}");

        int selection_start = ::SendMessage(h_scintilla, SCI_GETSELECTIONSTART, 0, 0);
        int selection_end   = ::SendMessage(h_scintilla, SCI_GETSELECTIONEND , 0, 0);

        int selection_length = selection_end - selection_start;

        if (selection_start == selection_end)
        {
            break;
        }
        else if (selection_length > 0 && selection_length != 4 && selection_length != 7)
        {
            ::SendMessage(h_scintilla, SCI_SETSELECTIONSTART, selection_end, 0);
            ::SendMessage(h_scintilla, SCI_SETSELECTIONEND, selection_end, 0);

            ::SendMessage(h_scintilla, SCI_SETANCHOR, selection_end, 0);
            //i++;
            continue;
        }

        int hex_length;

        char selection[8];
        ::SendMessage(h_scintilla, SCI_GETSELTEXT , 0, (LPARAM)&selection);
		selection[7] = '\0';

        ::SendMessage(h_scintilla, SCI_SETSELECTIONSTART, selection_end, 0);
        ::SendMessage(h_scintilla, SCI_SETSELECTIONEND, selection_end, 0);

        ::SendMessage(h_scintilla, SCI_SETANCHOR, selection_end, 0);

        char hex[7];
        strcpy(hex, substr(selection, 1, strlen(selection)));

        if (strlen(hex) == 3)
        {
            hex[0] = selection[1];
            hex[1] = selection[1];
            hex[2] = selection[2];
            hex[3] = selection[2];
            hex[4] = selection[3];
            hex[5] = selection[3];
			hex[6] = '\0';

            hex_length = 3;
        }
        else {
            hex_length = 6;
        }

        // Reverse RGB to BGR which Scintilla uses
		char reverse[7];
		
        reverse[0] = toupper(hex[4]);
        reverse[1] = toupper(hex[5]);
        reverse[2] = toupper(hex[2]);
        reverse[3] = toupper(hex[3]);
        reverse[4] = toupper(hex[0]);
        reverse[5] = toupper(hex[1]);
		reverse[6] = '\0';

        indicator = i;
        
        char search[9];
        strcpy(search, "|");
        strcat(search, reverse);

		COLORREF color = strtol(reverse, NULL, 16);

        if (strstr(found, search)) {
            i--;
            int j = 0;
            while (j <= set_colors) {
                if (color == ::SendMessage(h_scintilla, SCI_INDICGETFORE, j, 0))
                {
                    indicator = j;
                    break;
                }
                else {
                    j++;
                }
            }
        }
        else {
            strcat(found, search);
            set_colors++;
        }

        ::SendMessage(h_scintilla, SCI_INDICSETSTYLE, indicator, INDIC_PLAIN);

        ::SendMessage(h_scintilla, SCI_INDICSETUNDER, indicator, (LPARAM)true);
        ::SendMessage(h_scintilla, SCI_INDICSETFORE, indicator, (LPARAM)color);
        ::SendMessage(h_scintilla, SCI_INDICSETALPHA, indicator, (LPARAM)255);

        ::SendMessage(h_scintilla, SCI_SETINDICATORCURRENT, indicator, 0);
        ::SendMessage(h_scintilla, SCI_INDICATORFILLRANGE, selection_start, hex_length +1);

        i++;
    }

    ::SendMessage(h_scintilla, SCI_SETSELECTIONSTART, save_caret_start, 0);
    ::SendMessage(h_scintilla, SCI_SETSELECTIONEND, save_caret_end, 0);

}

void RemoveColorHighlight() {

    HWND h_scintilla = GetScintilla();
    
    int length = ::SendMessage(h_scintilla, SCI_GETTEXT, 0, 0);
    int i = 1;
    while (i <= 20) {
        ::SendMessage(h_scintilla, SCI_SETINDICATORCURRENT, i, 0);
        ::SendMessage(h_scintilla, SCI_INDICATORCLEARRANGE, 0, length);
        i++;
    }

}

char* substr(char* string, int begin, int end) {

    char* temp = (char*)malloc(sizeof(char)*(end-begin+1));

    if (temp != NULL) {
        strncpy(temp, string+begin, end-begin+1);
        temp[end-begin+1] = '\0';
    }

    return temp;

}
