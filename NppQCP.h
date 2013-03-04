
#ifndef PLUGINDEFINITION_H
#define PLUGINDEFINITION_H

#include "PluginInterface.h"

const TCHAR NPP_PLUGIN_NAME[] = TEXT("Quick Color Picker +");
const TCHAR NPP_PLUGIN_VER[] = TEXT("1.0");

const int kCommandCount = 3;

void PluginInit(HANDLE module);
void PluginCleanUp();


void LoadConfig();
void SaveConfig();

void InitCommandMenu();
void commandMenuCleanUp();
bool setCommand(size_t index, TCHAR *cmdName, PFUNCPLUGINCMD pFunc, ShortcutKey *sk = NULL, bool check0nInit = false);


////////////////////////////////////////////
void ToggleColorCodeHighlight();
void VisitWebsite();

HWND GetScintilla();

void CreateMessageWindow();
void DestroyMessageWindow();
LRESULT CALLBACK MessageWindowWINPROC(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

void CreateColorPicker();
bool ShowColorPicker();
void HideColorPicker();
void ToggleColorPicker();
void WriteColorCodeToEditor(COLORREF color);

bool is_hex(char* str);
bool pad_hex(char* out, const char* in);

COLORREF revert_color_order(COLORREF color);
COLORREF hex2color(char* str);

void HighlightColorCode();
void RemoveColorHighlight();
char* substr(char* string, int begin, int end);


#endif //PLUGINDEFINITION_H
