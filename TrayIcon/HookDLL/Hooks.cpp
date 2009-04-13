// Hooks.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include <stdlib.h>
#include <objbase.h>
#include <Shellapi.h>

// constant /////////////////////////////////////////////////////////////////////////
//
#define SC_TRAYME	2
#define TRAYICONID	4

// struct ///////////////////////////////////////////////////////////////////////////
//
struct WNDDATA{
	HWND			m_hWnd;
	NOTIFYICONDATA	m_niData;
	BOOL			m_bHooked;
	BOOL			m_bMinimized;
};

// define share section for all hooked process //////////////////////////////////////
//
#pragma data_seg(".SHARE")
HHOOK			g_hInitMenuHook = NULL;
HHOOK			g_hMenuCommandHook = NULL;
WNDDATA			g_listWnd[255];
UINT			g_iLastIndex = 0;
UINT			SWM_TRAYMSG = 0; // it starts as this but is reassigned by windows as a unique message number
#pragma data_seg()
#pragma comment(linker,"/SECTION:.SHARE,RWS")

// global variables /////////////////////////////////////////////////////////////////
//
HINSTANCE	ghModule	= NULL;

// function prototype ///////////////////////////////////////////////////////////////
//
LRESULT CALLBACK InitMenuHookProc(int nCode, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK MenuCommandHookProc(int nCode, WPARAM wParam, LPARAM lParam);
HICON GetFileIconHandle(LPCTSTR lpszFileName, BOOL bSmallIcon);

/*********************************************************************
*	Function: DllMain
*	Author:	Chau Nguyen
*	Created On: 2003/10/24
* --------------------------------------------------------------------
*	Purpose: Entry point for DLL.
* --------------------------------------------------------------------
*	Input Parameters: No
*
*	Output Parameters: No
*
*	Return Value: No
*
* --------------------------------------------------------------------
* NOTE:
*	- 
*
* --------------------------------------------------------------------
* REVISION HISTORY:
*	Version		Date		Author		Description
*
*********************************************************************/
BOOL APIENTRY DllMain( HANDLE hModule, 
					  DWORD  ul_reason_for_call, 
					  LPVOID lpReserved
					  )
{
	// reserve the DLL handle
	ghModule = (HINSTANCE)hModule;

	// register system-wide message
	SWM_TRAYMSG = RegisterWindowMessage("MY_HOOK_MESSAGE");

	//
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

/*********************************************************************
*	Function: BOOL WINAPI InstallHook()
*	Author:	Chau Nguyen
*	Created On: 2003/10/24
* --------------------------------------------------------------------
*	Purpose: Install hooks
* --------------------------------------------------------------------
*	Input Parameters: No
*
*	Output Parameters: No
*
*	Return Value: TRUE : success, FALSE : fail
*
* --------------------------------------------------------------------
* NOTE:
*	- 
*
* --------------------------------------------------------------------
* REVISION HISTORY:
*	Version		Date		Author		Description
*
*********************************************************************/
BOOL WINAPI InstallHook()
{
	g_hInitMenuHook = SetWindowsHookEx(WH_CALLWNDPROC, InitMenuHookProc, ghModule, 0);
	if( g_hInitMenuHook == NULL ){
		return FALSE;	
	}
	/*
	g_hMenuCommandHook = SetWindowsHookEx(WH_GETMESSAGE, MenuCommandHookProc, ghModule, 0);
	if( g_hMenuCommandHook == NULL ){
	return FALSE;	
	}
	*/


	return TRUE;
}

/*********************************************************************
*	Function: BOOL WINAPI UnInstallHook()
*	Author:	Chau Nguyen
*	Created On: 2003/10/24
* --------------------------------------------------------------------
*	Purpose: Uninstall hooks
* --------------------------------------------------------------------
*	Input Parameters: No
*
*	Output Parameters: No
*
*	Return Value: No
*
* --------------------------------------------------------------------
* NOTE:
*	- 
*
* --------------------------------------------------------------------
* REVISION HISTORY:
*	Version		Date		Author		Description
*
*********************************************************************/
BOOL WINAPI UnInstallHook()
{
	BOOL bSuccess = FALSE;

	// free all minimized windows
	/*
	broken currently see discussion for http://www.codeproject.com/KB/system/MinimizedAnyWindowToTray.aspx?msg=2996343#xx2996343xx
	for(UINT i = 0; i < g_iLastIndex; i++){
	if(g_listWnd[i].m_bMinimized){
	Shell_NotifyIcon(NIM_DELETE, &g_listWnd[i].m_niData);
	ShowWindow(g_listWnd[i].m_hWnd, SW_SHOW);
	g_listWnd[i].m_bMinimized = FALSE;
	}
	} */
	
	// unhook InitMenu
	bSuccess = UnhookWindowsHookEx(g_hInitMenuHook);
	if(!bSuccess){
		return FALSE;
	}

	// unhook MenuCommmand
	bSuccess = UnhookWindowsHookEx(g_hMenuCommandHook);
	if(!bSuccess){
		return FALSE;
	}

	return TRUE;
}

// TODO first time never says 'tray me'

/*********************************************************************
*	Function: LRESULT CALLBACK InitMenuHookProc(int nCode, WPARAM wParam, LPARAM lParam)
*	Author:	Chau Nguyen
*	Created On: 2003/10/24
* --------------------------------------------------------------------
*	Purpose: Intercept InitPopupMenu message to change menu before its displaying
* --------------------------------------------------------------------
*	Input Parameters: No
*
*	Output Parameters: No
*
*	Return Value: No
*
* --------------------------------------------------------------------
* NOTE:
*	- 
*
* --------------------------------------------------------------------
* REVISION HISTORY:
*	Version		Date		Author		Description
*
*********************************************************************/




FILE *fp = 0;
void debugOut(char *message) {
	if(!fp)
		fp=fopen("c:\\test.txt", "a");
	fprintf(fp, message);
	fprintf(fp, "\n");
	OutputDebugString(message);
	fflush(fp);
	//fclose(fp);
}
RECT hRgn;
RECT hRgn2;

// from http://simplesamples.info/Windows/EnumWindows.php
// with most of the original code from http://www.codeproject.com/KB/system/MinimizedAnyWindowToTray.aspx
// both seem license free
// this code release (C) 2009 roger Pack released MIT license
char message[250];
BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam) {
	DWORD dwThreadId, dwProcessId;
	HINSTANCE hInstance;
	char String[255];
	HANDLE hProcess;
	if (!hWnd)
		return TRUE;		// Not a window
	if (!::IsWindowVisible(hWnd))
		return TRUE;		// Not visible
	if (!SendMessage(hWnd, WM_GETTEXT, sizeof(String), (LPARAM)String))
		return TRUE;		// No window title
	hInstance = (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE);
	dwThreadId = GetWindowThreadProcessId(hWnd, &dwProcessId);

	GetWindowRect(hWnd, &hRgn2);
	// did it match?
	// was it "close to the top so probably from right clicking on the top bar"
	// was it "not from clicking on the top leftmost icon, approximately?
	if(hRgn2.top > 0 && hRgn2.bottom >= hRgn.bottom && hRgn2.left >= hRgn.left && hRgn2.right <= hRgn.right && hRgn2.bottom <= hRgn.bottom) {
		
		sprintf(message, "WINNER sending wm close to window %x", hWnd);
		debugOut(message);
		sprintf(message, "hrgn:%d,%d,%d,%d", hRgn2.bottom, hRgn2.left, hRgn2.right, hRgn2.top);
		debugOut(message);
		// check if it "seems like" we clicked on the top bar
		POINT oldMouse;
		GetCursorPos(&oldMouse);
		// 191 versus 164 -> 27
		int topMenuSize = 25;
		LONG currentX = oldMouse.x;
		LONG windowsX = hRgn2.left;
		// XXXX cleanup
		// TODO not write to that file :)
		// can release it before then tho
		// XXXX clicking on the bottom should close auto, too :)
		// XXXX double right click to close from anywhere :)
		// XXXX not send two WM_CLOSE messages to the same handle twice in a row.
		// XXXX work with cmd.exe windows [possible?]
		// XXXX prevent drop down from appearing [? possible ?]
		if(oldMouse.y > hRgn2.top && (oldMouse.y - hRgn2.top) < topMenuSize &&  ((currentX - windowsX) > topMenuSize)) {
			debugOut("true winner\n");
			
			INPUT *buffer = new INPUT[3]; //allocate a buffer
			int X;
			int Y;
			X = hRgn2.left + 10;
			Y = hRgn2.top + 10;	
			sprintf(message, "trying to move to %d, %d", X, Y);
			debugOut(message);

			(buffer+1)->type = INPUT_MOUSE;
			(buffer+1)->mi.dx = 100;
			(buffer+1)->mi.dy = 100;
			(buffer+1)->mi.mouseData = 0;
			(buffer+1)->mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
			(buffer+1)->mi.time = 0;
			(buffer+1)->mi.dwExtraInfo = 0;
//TODO doesn't work for maximized
			//TODO sends redundant messages to netbeans
			//LDO options for whether to be aggressive or not
			// TODO new icon
			// TODO allow for upper left to "not" close it...I guess :P
			// TODO allow for variable size title bars [with mine it can err because too aggressive
			(buffer+2)->type = INPUT_MOUSE;
			(buffer+2)->mi.dx = 100;
			(buffer+2)->mi.dy = 100;
			(buffer+2)->mi.mouseData = 0;
			(buffer+2)->mi.dwFlags = MOUSEEVENTF_LEFTUP;
			(buffer+2)->mi.time = 0;
			(buffer+2)->mi.dwExtraInfo = 0;

			POINT oldMouse;
			GetCursorPos(&oldMouse);
			SetCursorPos(X,Y);
			SendInput(3,buffer,sizeof(INPUT));
			SendInput(3,buffer,sizeof(INPUT));
			SendMessage(hWnd, WM_CLOSE, NULL, NULL); // send the [ineffective usually] WM_CLOSE message
			// in case double click didn't work.
			SetCursorPos(oldMouse.x, oldMouse.y);

			PostThreadMessage(dwThreadId, WM_CLOSE, NULL, NULL);
			//TODO do we want more messages?
			delete (buffer); //clean up our messes.
		}	
	}


	return TRUE;
}

LRESULT CALLBACK InitMenuHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{

	// return immediately if system required
	if (nCode < 0){
		return CallNextHookEx(g_hInitMenuHook, nCode, wParam, lParam); 
	}
	if(fp == 0) {
		debugOut("WM_MENUSELECT");
	}

	// get window procedure struct
	CWPSTRUCT *wps = (CWPSTRUCT*)lParam;

	// check if this message is not the my own registered message (it is a system message)
	if(wps->message != SWM_TRAYMSG) {		

		// not my message
		switch (wps->message) {

case WM_MENUSELECT: // something from the menu was selected
	// menu is being closed -- cleanup
	break;
	debugOut("WM_MENUSELECT");
	debugOut("here1--message is");
	sprintf(message, "message was %d", wps->message);
	debugOut(message);


	if(wps->lParam == NULL && HIWORD(wps->wParam) == 0xFFFF){
		// determine what window
		for(UINT i = 0; i < g_iLastIndex; i++){
			if(g_listWnd[i].m_hWnd == (HWND)wps->hwnd){
				// remove menu item if it was added
				if(g_listWnd[i].m_bHooked){
					RemoveMenu(GetSystemMenu(g_listWnd[i].m_hWnd, FALSE), SC_TRAYME, MF_BYCOMMAND);
				}

				break;
			}
		}
	}
	break;
case WM_INITMENUPOPUP:{ // pre menu popup
	OutputDebugString("\nmenu initmenupopup\n");

	GetWindowRect(wps->hwnd, &hRgn);
	sprintf(message, "hrgn STARTED:%d,%d,%d,%d", hRgn.bottom, hRgn.left, hRgn.right, hRgn.top);
	debugOut(message);

	DWORD processId;

	GetWindowThreadProcessId(wps->hwnd, &processId);
	fprintf(fp, "pid started:%d\n", processId );		
	//SendMessage(wps->hwnd, WM_CLOSE, NULL, NULL);
	//PostMessage(wps->hwnd, WM_CLOSE, 0, 0);
	//SendMessage(wps->hwnd,0x0112,0xF060,0);
	EnumWindows(EnumWindowsProc, NULL);
	break;
	// menu is being opened -- add ourselves to it
	// get the menu handle
	HMENU hMenu = (HMENU)wps->wParam;

	// check if it is a menu
	if(IsMenu(hMenu)){
		OutputDebugString("\nisMenu is true\n");
		// is system menu
		sprintf(message, "wps is hwnd:%d, lparam:%d, message:%d, wparam:%d, parent:%d", wps->hwnd, wps->lParam, wps->message,
			wps->wParam, GetParent(wps->hwnd));
		OutputDebugString(message);

		if(HIWORD(wps->lParam) == TRUE){ 
			UINT i=0;
			// check if this window was hooked or not
			for(i = 0; i < g_iLastIndex; i++){
				if(g_listWnd[i].m_hWnd == (HWND)wps->hwnd){
					break; // exit the for loop
				}
			}

			// not hooked yet
			if(i == g_iLastIndex){
				// store the window handle
				g_listWnd[i].m_hWnd = (HWND)wps->hwnd;
				g_listWnd[i].m_bMinimized = FALSE;

				// get title od that window
				char szText[255];
				GetWindowText(g_listWnd[i].m_hWnd, szText, 255);

				// prepare a NotifyData struct for this winfow
				ZeroMemory(&(g_listWnd[i].m_niData), sizeof(NOTIFYICONDATA));
				g_listWnd[i].m_niData.cbSize = sizeof(NOTIFYICONDATA);
				g_listWnd[i].m_niData.hWnd = (HWND)wps->hwnd;

				HICON hIcon = (HICON)SendMessage(g_listWnd[i].m_hWnd, WM_GETICON, ICON_SMALL,0);					
				if(!hIcon){
					char szPath[255];
					HMODULE hModule = (HMODULE)OpenProcess(0, FALSE, GetWindowThreadProcessId(g_listWnd[i].m_hWnd, 0));
					GetModuleFileName(hModule, szPath, 255);
					hIcon = GetFileIconHandle(szPath, TRUE);
				}


				if(hIcon){
					g_listWnd[i].m_niData.hIcon = CopyIcon(hIcon);
				}
				else{
					g_listWnd[i].m_niData.hIcon = LoadIcon(NULL, IDI_QUESTION);
				}
				g_listWnd[i].m_niData.uID = TRAYICONID;
				g_listWnd[i].m_niData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
				strcpy(g_listWnd[i].m_niData.szTip, szText);
				g_listWnd[i].m_niData.uCallbackMessage = SWM_TRAYMSG;

				// append our own menu item
				AppendMenu(GetSystemMenu(g_listWnd[i].m_hWnd, FALSE), MF_BYPOSITION|MF_STRING, SC_TRAYME, "&Tray Me");					

				// set the flag on
				g_listWnd[i].m_bHooked = TRUE;

				// increase count
				g_iLastIndex++;
			}
			else if(i < g_iLastIndex){// hook allready
				// append our own menu item
				AppendMenu(GetSystemMenu(g_listWnd[i].m_hWnd, FALSE), MF_BYPOSITION|MF_STRING, SC_TRAYME, "&Tray Me");					

				// set the flag on
				g_listWnd[i].m_bHooked = TRUE;
			}
		}
	}
	break;
					  }			

		}

	}
	else{ // this is my registered message
		switch(wps->lParam)
		{
		case WM_LBUTTONDOWN:
			// determine what window
			for(UINT i = 0; i < g_iLastIndex; i++){
				if(g_listWnd[i].m_hWnd == (HWND)wps->hwnd){
					Shell_NotifyIcon(NIM_DELETE, &g_listWnd[i].m_niData);
					ShowWindow(g_listWnd[i].m_hWnd, SW_SHOW);

					g_listWnd[i].m_bMinimized = FALSE;
					break;
				}
			}

			break;
		}
	}

	return CallNextHookEx(g_hInitMenuHook, nCode, wParam, lParam); 
}


/*********************************************************************
*	Function: LRESULT CALLBACK MenuCommandHookProc(int nCode, WPARAM wParam, LPARAM lParam)
*	Author:	Chau Nguyen
*	Created On: 2003/10/24
* --------------------------------------------------------------------
*	Purpose: Intercept all command message to process my own menu command.
* --------------------------------------------------------------------
*	Input Parameters: No
*
*	Output Parameters: No
*
*	Return Value: No
*
* --------------------------------------------------------------------
* NOTE:
*	- 
*
* --------------------------------------------------------------------
* REVISION HISTORY:
*	Version		Date		Author		Description
*
*********************************************************************/
LRESULT CALLBACK MenuCommandHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	// return immediately if system required
	if (nCode < 0){
		return CallNextHookEx(g_hMenuCommandHook, nCode, wParam, lParam); 
	}

	// process the arrival message
	MSG *msg = (MSG*)lParam;
	switch (msg->message) {
case WM_SYSCOMMAND:
	if(LOWORD(msg->wParam) == SC_TRAYME ){
		// iterate to determine the arrival command belongs to which window
		for(UINT i = 0; i < g_iLastIndex; i++){
			if(g_listWnd[i].m_hWnd == (HWND)msg->hwnd){
				// check if it was not minimized yet
				if(!g_listWnd[i].m_bMinimized){
					// Add icon to tray
					Shell_NotifyIcon(NIM_ADD, &g_listWnd[i].m_niData);

					// free icon handle


					// hide window
					ShowWindow(g_listWnd[i].m_hWnd, SW_HIDE);

					// set the flag
					g_listWnd[i].m_bMinimized = TRUE;
				}

				break; // exit the for loop
			}
		}
	}
	break;
	}

	// call next hook
	return CallNextHookEx(g_hMenuCommandHook, nCode, wParam, lParam); 
}


HICON GetFileIconHandle(LPCTSTR lpszFileName, BOOL bSmallIcon)
{
	UINT uFlags = SHGFI_ICON | SHGFI_USEFILEATTRIBUTES;

	if (bSmallIcon)
		uFlags |= SHGFI_SMALLICON;
	else
		uFlags |= SHGFI_LARGEICON;

	SHFILEINFO sfi;
	SHGetFileInfo(lpszFileName, FILE_ATTRIBUTE_NORMAL, &sfi, sizeof(SHFILEINFO), uFlags);

	return sfi.hIcon;
}

