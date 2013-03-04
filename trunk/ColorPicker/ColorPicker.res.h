#ifndef COLOR_PICKER_RESOURCE_H
#define COLOR_PICKER_RESOURCE_H

#define IDI_PICKER 101
#define IDI_CURSOR 102

#define	IDD_COLOR_PICKER_POPUP   2110
#define	IDC_COLOR_PALETTE		(IDD_COLOR_PICKER_POPUP + 1)
#define IDC_COLOR_TEXT			(IDD_COLOR_PICKER_POPUP + 2)
#define ID_PICK					(IDD_COLOR_PICKER_POPUP + 3)
#define ID_MORE					(IDD_COLOR_PICKER_POPUP + 4)
#define IDC_OLD_COLOR			(IDD_COLOR_PICKER_POPUP + 5)
#define IDC_NEW_COLOR			(IDD_COLOR_PICKER_POPUP + 6)
#define IDC_COLOR_BG			(IDD_COLOR_PICKER_POPUP + 7)

#define WM_COLOR_PICKER_POPUP_BASE	WM_USER + 100
#define WM_PICKUP_COLOR				(WM_COLOR_PICKER_POPUP_BASE + 1)
#define WM_PICKUP_CANCEL			(WM_COLOR_PICKER_POPUP_BASE + 2)

#define	IDD_SCREEN_PICKER_POPUP   2210
#define IDC_SCR_OLD_COLOR		(IDD_SCREEN_PICKER_POPUP + 1)
#define IDC_SCR_NEW_COLOR		(IDD_SCREEN_PICKER_POPUP + 2)
#define IDC_SCR_COLOR_HEX			(IDD_SCREEN_PICKER_POPUP + 3)
#define IDC_SCR_COLOR_RGB			(IDD_SCREEN_PICKER_POPUP + 4)
#define IDC_SCR_ZOOM_AREA			(IDD_SCREEN_PICKER_POPUP + 5)

#define WM_SCREEN_HOVER_COLOR			(WM_COLOR_PICKER_POPUP_BASE + 3)
#define WM_SCREEN_PICK_COLOR			(WM_COLOR_PICKER_POPUP_BASE + 4)
#define WM_SCREEN_PICK_CANCEL			(WM_COLOR_PICKER_POPUP_BASE + 5)

#endif //COLOR_PICKER_RESOURCE_H
