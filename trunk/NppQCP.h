//this file is part of notepad++
//Copyright (C)2003 Don HO <donho@altern.org>
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#ifndef PLUGINDEFINITION_H
#define PLUGINDEFINITION_H

//
// All difinitions of plugin interface
//
#include "PluginInterface.h"

//-------------------------------------//
//-- STEP 1. DEFINE YOUR PLUGIN NAME --//
//-------------------------------------//
// Here define your plugin name
//
const TCHAR NPP_PLUGIN_NAME[] = TEXT("Quick Color Picker +");
const TCHAR NPP_PLUGIN_VER[] = TEXT("1.0");

//-----------------------------------------------//
//-- STEP 2. DEFINE YOUR PLUGIN COMMAND NUMBER --//
//-----------------------------------------------//
//
// Here define the number of your plugin commands
//
const int kCommandCount = 3;


//
// Initialization of your plugin data
// It will be called while plugin loading
//
void PluginInit(HANDLE module);

//
// Cleaning of your plugin
// It will be called while plugin unloading
//
void PluginCleanUp();

//
//Initialization of your plugin commands
//
void InitCommandMenu();

//
//Clean up your plugin commands allocation (if any)
//
void commandMenuCleanUp();

//
// Function which sets your command 
//
bool setCommand(size_t index, TCHAR *cmdName, PFUNCPLUGINCMD pFunc, ShortcutKey *sk = NULL, bool check0nInit = false);


//
// Your plugin command functions
//
void ToggleColorCodeHighlight();
void VisitWebsite();

HWND GetScintilla();

void CreateMessageWindow();
LRESULT CALLBACK MessageWindowWINPROC(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

bool ShowColorPicker();
void HideColorPicker();
void WriteColorCodeToEditor(COLORREF color);

bool is_hex(char* str);
bool pad_hex(char* out, const char* in);

COLORREF revert_color_order(COLORREF color);
COLORREF hex2color(char* str);

void HighlightColorCode();
void RemoveColorHighlight();
char* substr(char* string, int begin, int end);


#endif //PLUGINDEFINITION_H
