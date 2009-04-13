// Win32 Dialog.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "resource.h"

#define HOOKDLL "HookDLL.dll"
#define TRAYICONID	1			//	ID number for the Notify Icon
#define SWM_TRAYMSG	WM_APP		//	the message ID sent to our window

#define SWM_ENABLED	WM_APP + 1	//	
#define SWM_EXIT	WM_APP + 2	//	close the window

typedef LRESULT (WINAPI *ATTACH_FUNCTION_POINTER)();
typedef LRESULT (WINAPI *DETACH_FUNCTION_POINTER)();

// Global Variables:
HINSTANCE		hInst;	// current instance
NOTIFYICONDATA	niData;	// notify icon data
HMODULE			hInjectDLL = NULL;
BOOL			bEnabled = FALSE;


// Forward declarations of functions included in this code module:
BOOL				InitInstance(HINSTANCE, int);
BOOL				OnInitDialog(HWND hWnd);
void				ShowContextMenu(HWND hWnd);

// function prototype
ATTACH_FUNCTION_POINTER		pfAttach;
DETACH_FUNCTION_POINTER		pfDetach;
INT_PTR CALLBACK			DlgProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK			About(HWND, UINT, WPARAM, LPARAM);


int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	MSG msg;

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow)) return FALSE;

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return (int) msg.wParam;
}

//	Initialize the window and tray icon
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	
	 // store instance handle and create dialog
	hInst = hInstance;
	HWND hWnd = CreateDialog( hInstance, MAKEINTRESOURCE(IDD_DLG_DIALOG), NULL, (DLGPROC)DlgProc );
	if (!hWnd) return FALSE;

	// Fill the NOTIFYICONDATA structure and call Shell_NotifyIcon
	ZeroMemory(&niData,sizeof(NOTIFYICONDATA));
	niData.cbSize = sizeof(NOTIFYICONDATA);
	niData.uID = TRAYICONID;
	niData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	niData.hIcon = (HICON)LoadImage(hInstance,MAKEINTRESOURCE(IDI_ICON),
									IMAGE_ICON, GetSystemMetrics(SM_CXSMICON),
									GetSystemMetrics(SM_CYSMICON),LR_DEFAULTCOLOR);
	strcpy(niData.szTip, "Tray Icon");
	niData.hWnd = hWnd;
    niData.uCallbackMessage = SWM_TRAYMSG;

	// Add icon to tray
	Shell_NotifyIcon(NIM_ADD, &niData);

	// free icon handle
	if(niData.hIcon && DestroyIcon(niData.hIcon)){
		niData.hIcon = NULL;
	}
		
	// call ShowWindow here to make the dialog initially visible

	// Load Inject DLL
	hInjectDLL = LoadLibrary(HOOKDLL);
	if(hInjectDLL != NULL){
		pfAttach = (ATTACH_FUNCTION_POINTER)GetProcAddress(hInjectDLL, "InstallHook");
		pfDetach = (DETACH_FUNCTION_POINTER)GetProcAddress(hInjectDLL, "UnInstallHook");

		if(pfAttach == NULL || pfDetach == NULL){
			MessageBox(NULL, "Could not get function pointer in hooking DLL", "TrayIcon", MB_OK);
		}
	}
	else{
		MessageBox(NULL, "Could not load the hooking DLL", "TrayIcon", MB_OK);
	}

	// inject
	if(hInjectDLL != NULL){
		if(pfAttach() == TRUE){
		bEnabled = TRUE;
		}
		else{
			bEnabled = FALSE;
		}
	}
	
	return TRUE;
}


BOOL OnInitDialog(HWND hWnd)
{
	return TRUE;
}

// Name says it all
void ShowContextMenu(HWND hWnd)
{
	POINT pt;
	GetCursorPos(&pt);
	HMENU hMenu = CreatePopupMenu();
	if(hMenu)
	{
		if(bEnabled){
			InsertMenu(hMenu, -1, MF_BYPOSITION, SWM_ENABLED, _T("Disable"));
		}
		else{
			InsertMenu(hMenu, -1, MF_BYPOSITION, SWM_ENABLED, _T("Enable"));
		}

		InsertMenu(hMenu, -1, MF_SEPARATOR, 0, NULL);
		InsertMenu(hMenu, -1, MF_BYPOSITION, SWM_EXIT, _T("Exit"));

		// note:	must set window to the foreground or the
		//			menu won't disappear when it should
		SetForegroundWindow(hWnd);

		TrackPopupMenu(hMenu, TPM_BOTTOMALIGN, pt.x, pt.y, 0, hWnd, NULL );
		
		DestroyMenu(hMenu);
	}
}


// Message handler for the app
INT_PTR CALLBACK DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;

	switch (message) 
	{
	case SWM_TRAYMSG:
		switch(lParam)
		{
		case WM_LBUTTONDBLCLK:
			DialogBox(hInst, (LPCTSTR)IDD_ABOUTBOX, hWnd, (DLGPROC)About);
			break;
		case WM_RBUTTONDOWN:
		case WM_CONTEXTMENU:
			ShowContextMenu(hWnd);
		}
		break;
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam); 

		switch (wmId)
		{
		case SWM_EXIT:
			DestroyWindow(hWnd);
			break;
		case SWM_ENABLED:
			if(bEnabled){
				if(pfDetach() == TRUE){
					bEnabled = FALSE;
				}
			}
			else{	
				if(pfAttach() == TRUE){
					bEnabled = TRUE;
				}
				else{
					bEnabled = FALSE;
					MessageBox(NULL, "Could not inject.", "TrayIcon", MB_ICONINFORMATION | MB_OK);					
				}
			}

			break;
		}
		return 1;
	case WM_INITDIALOG:
		return OnInitDialog(hWnd);
	case WM_CLOSE:
		DestroyWindow(hWnd);
		break;
	case WM_DESTROY:
		niData.uFlags = 0;
		Shell_NotifyIcon(NIM_DELETE,&niData);
		PostQuitMessage(0);

		// detach if not detach yet
		if(bEnabled){
			pfDetach();
		}
		FreeLibrary(hInjectDLL);
		break;
	}
	return 0;
}

// Message handler for about box.
LRESULT CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		return TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) 
		{
			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		}
		break;
	}
	return FALSE;
}
