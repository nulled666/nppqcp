// Quick Color Picker plugin for Notepad++

#include "NppQCP.h"

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <shlwapi.h>
#include <strsafe.h>

#include "ColorPicker\ColorPicker.h"
#include "ColorPicker\ColorPicker.res.h"


const TCHAR kIniSection[] = TEXT("QCP");
const TCHAR kIniKey[] = TEXT("enabled");
const TCHAR kConfigFileName[] = TEXT("npp_plugin_qcp.ini");


//
// The data of Notepad++ that you can use in your plugin commands
// extern variables - don't change the name
NppData nppData;
FuncItem funcItem[kCommandCount];
bool doCloseTag;


TCHAR _ini_file_path[MAX_PATH];
bool _enable_color_code_highlight = false;


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

	::WritePrivateProfileString(
		kIniSection, kIniKey,
		_enable_color_code_highlight ? TEXT("1") : TEXT("0") ,
		_ini_file_path
	);

	if (_pColorPicker) {
		::SendMessage(nppData._nppHandle, NPPM_MODELESSDIALOG, MODELESSDIALOGADD, (LPARAM)_pColorPicker->GetWindow());
	}

}

//
// Initialization of your plugin commands
// You should fill your plugins commands here
void InitCommandMenu() {

	// get path of plugin configuration
	::SendMessage(nppData._nppHandle, NPPM_GETPLUGINSCONFIGDIR, MAX_PATH, (LPARAM)_ini_file_path);

	// if config path doesn't exist, we create it
	if (PathFileExists(_ini_file_path) == FALSE)
	{
		::CreateDirectory(_ini_file_path, NULL);
	}

	// make your plugin config file full file path name
	PathAppend(_ini_file_path, kConfigFileName);

	// get the parameter value from plugin config
	_enable_color_code_highlight = (::GetPrivateProfileInt(kIniSection, kIniKey, 0, _ini_file_path) != 0);


    //--------------------------------------------//
    //-- STEP 3. CUSTOMIZE YOUR PLUGIN COMMANDS --//
    //--------------------------------------------//
    // with function :
    // setCommand(int index,                      // zero based number to indicate the order of command
    //            TCHAR *commandName,             // the command name that you want to see in plugin menu
    //            PFUNCPLUGINCMD functionPointer, // the symbol of function (function pointer) associated with this command. The body should be defined below. See Step 4.
    //            ShortcutKey *shortcut,          // optional. Define a shortcut to trigger this command
    //            bool check0nInit                // optional. Make this menu item be checked visually
    //            );

	TCHAR text[200] = TEXT("Visit Website ");
	SIZE_T size = ARRAYSIZE(text);
	StringCchCat(text, size, TEXT(" (Version "));
	StringCchCat(text, size, NPP_PLUGIN_VER);
	StringCchCat(text, size, TEXT(")"));

	setCommand(0, TEXT("Highlight Color Code for this File Type"), ToggleColorCodeHighlight, NULL, _enable_color_code_highlight);
	setCommand(1, TEXT("---"), NULL, NULL, false);
	setCommand(2, text, VisitWebsite, NULL, FALSE);

}


//
// This function help you to initialize your plugin commands
//
bool setCommand(size_t index, TCHAR *cmdName, PFUNCPLUGINCMD pFunc, ShortcutKey *sk, bool checkOnInit) {
    if (index >= kCommandCount)
        return false;

    if (!pFunc)
        return false;

    StringCchCopy(funcItem[index]._itemName, ARRAYSIZE(funcItem[index]._itemName), cmdName);
    funcItem[index]._pFunc = pFunc;
    funcItem[index]._init2Check = checkOnInit;
    funcItem[index]._pShKey = sk;

    return true;
}


//
// Here you can do the clean up (especially for the shortcut)
//
void commandMenuCleanUp() {
}

//----------------------------------------------//
//-- STEP 4. DEFINE YOUR ASSOCIATED FUNCTIONS --//
//----------------------------------------------//

// toggle color code highlight
void ToggleColorCodeHighlight() {

	_enable_color_code_highlight = !_enable_color_code_highlight;
	::CheckMenuItem(::GetMenu(nppData._nppHandle), funcItem[0]._cmdID, MF_BYCOMMAND | (_enable_color_code_highlight?MF_CHECKED:MF_UNCHECKED));

}

// visit nppqcp website
void VisitWebsite() {

	TCHAR url[200] = TEXT("http://www.github.com");
	ShellExecute(NULL, TEXT("open"), url, NULL, NULL, SW_SHOWNORMAL);

}

// create a background window for message communication
void CreateMessageWindow() {
	
	static TCHAR szWindowClass[] = TEXT("npp_qcp_msgwin");
	
	WNDCLASSEX wc    = {0};
	wc.cbSize        = sizeof(wc);
	wc.lpfnWndProc   = MessageWindowWINPROC;
	wc.cbClsExtra     = 0;
    wc.cbWndExtra     = 0;
    wc.hInstance      = _instance;
	wc.lpszClassName = szWindowClass;

	if (!RegisterClassEx(&wc)) {
		throw std::runtime_error("NppQCP: RegisterClassEx() failed");
	}

	_message_window = CreateWindowEx(0, szWindowClass, szWindowClass, WS_POPUP,	0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL);

	if (!_message_window) {
		throw std::runtime_error("NppQCP: CreateWindowEx() function returns null");
	}

}

// message proccessor
LRESULT CALLBACK MessageWindowWINPROC(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {

	switch (message) {
		case WM_PICKUP_COLOR: {
			//HideColorPicker();
			COLORREF color = (COLORREF)wparam;
			WriteColorCodeToEditor(color);
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

bool ShowColorPicker(){

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

	char hex_color[10];
    ::SendMessage(h_scintilla, SCI_GETSELTEXT , 0, (LPARAM)&hex_color);

	// break - not a hex string
	if (!is_hex(hex_color))
		return false;

	// passed -

	// padding the 3 char hex into 6 char hex
	if (len == 3){
		char out[7];
		if(!pad_hex(out, hex_color))
			return false;
		strcpy(hex_color, out);
	}

	// convert hex string to COLORREF
	COLORREF color = hex2color(hex_color);

	// prepare coordinates
	POINT p;
	p.x = ::SendMessage(h_scintilla, SCI_POINTXFROMPOSITION , 0, selection_start);
	p.y = ::SendMessage(h_scintilla, SCI_POINTYFROMPOSITION , 0, selection_start);
	::ClientToScreen(h_scintilla, &p);

	int lineHeight = ::SendMessage(h_scintilla, SCI_TEXTHEIGHT , 0, 1); // all the same

	// create the color picker if not created
	if (!_pColorPicker) {
		_pColorPicker = new ColorPicker();
		_pColorPicker->Create(_instance, nppData._nppHandle, _message_window);
		::SendMessage(nppData._nppHandle, NPPM_MODELESSDIALOG, MODELESSDIALOGADD, (LPARAM)_pColorPicker->GetWindow());
	}

	// set and show
	_pColorPicker->Color(color);
	_pColorPicker->PlaceWindow(p, lineHeight);
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


void WriteColorCodeToEditor(COLORREF color) {

	// REVERT FOR OUTPUT
	color = RGB(GetBValue(color),GetGValue(color),GetRValue(color));

	char buff[10];
	sprintf(buff, "%0*X", 6, color);

	// SCI_REPLACESEL only accept char*
	HWND current_scintilla = GetScintilla();
	::SendMessage(current_scintilla, SCI_REPLACESEL, NULL, (LPARAM)(char*)buff);

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

	int lang = -100;
    ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTLANGTYPE, 0, (LPARAM)&lang);
    
    RemoveColorHighlight();

    if (lang != L_CSS) {
        return;
    }

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
