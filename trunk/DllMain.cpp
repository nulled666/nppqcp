
#include "stdio.h"
#include <shlwapi.h>

#include "NppQCP.h"


extern FuncItem funcItem[kCommandCount];
extern NppData nppData;
extern bool doCloseTag;


BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  reasonForCall, 
                       LPVOID lpReserved ) {

    switch (reasonForCall) {
      case DLL_PROCESS_ATTACH:
        PluginInit(hModule);
        break;
      case DLL_PROCESS_DETACH:
        PluginCleanUp();
        break;
      case DLL_THREAD_ATTACH:
        break;
      case DLL_THREAD_DETACH:
        break;
    }

    return TRUE;

}


extern "C" __declspec(dllexport) void setInfo(NppData notpadPlusData) {
	nppData = notpadPlusData;
	InitCommandMenu();
}

extern "C" __declspec(dllexport) const TCHAR * getName() {
	return NPP_PLUGIN_NAME;
}

extern "C" __declspec(dllexport) FuncItem * getFuncsArray(int *nbF) {
	*nbF = kCommandCount;
	return funcItem;
}


bool _is_color_picker_shown = false;

extern "C" __declspec(dllexport) void beNotified(SCNotification *notifyCode) {

	switch (notifyCode->nmhdr.code) {
		case NPPN_SHUTDOWN: {
			commandMenuCleanUp();
			break;
		}
		case SCN_UPDATEUI: {
			if (notifyCode->updated & SC_UPDATE_V_SCROLL) {
				HighlightColorCode();
			}
			if (notifyCode->updated & SC_UPDATE_SELECTION ) {
				if (!_is_color_picker_shown ){
					_is_color_picker_shown = ShowColorPicker();
				}else{
					HideColorPicker();
					_is_color_picker_shown = false;
				}
			}
			break;
		}
		case SCN_ZOOM:
		case SCN_SCROLLED: {
			HideColorPicker();
			break;
		}
		case SCN_MODIFIED: {
			if (notifyCode->modificationType & SC_MOD_DELETETEXT
				|| notifyCode->modificationType & SC_MOD_INSERTTEXT) {
                HighlightColorCode();
            }
            break;
		}		
		case NPPN_LANGCHANGED:
		case NPPN_BUFFERACTIVATED: {
            HighlightColorCode();
            break;
		}
		default: {
			// nothing
		}
	}

}


// Here you can process the Npp Messages 
// I will make the messages accessible little by little, according to the need of plugin development.
// Please let me know if you need to access to some messages :
// http://sourceforge.net/forum/forum.php?forum_id=482781
//
extern "C" __declspec(dllexport) LRESULT messageProc(UINT message, WPARAM wParam, LPARAM lParam) {
	
	if (message == WM_MOVE) {
		HideColorPicker();
	}
	return TRUE;

}

#ifdef UNICODE
extern "C" __declspec(dllexport) BOOL isUnicode() {
    return TRUE;
}
#endif //UNICODE
