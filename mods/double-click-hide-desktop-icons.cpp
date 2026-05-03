// ==WindhawkMod==
// @id              double-click-hide-desktop-icons
// @name            Double Click hide Desktop Icons
// @description     Hides or shows the desktop icons by double-clicking on an empty space.
// @version         1.0.3
// @author          Gemini
// @include         explorer.exe
// @compilerOptions -lcomctl32
// ==/WindhawkMod==

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <stdlib.h>

HHOOK g_hMouseHook = NULL;
HWND g_hwndDefView = NULL;
HWND g_hwndDesktopList = NULL;
const int TOGGLE_ICONS_COMMAND = 0x7402;

ULONGLONG g_lastClickTime = 0;
POINT g_lastClickPos = {0, 0};

LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && (wParam == WM_LBUTTONDOWN || wParam == WM_LBUTTONDBLCLK)) {
        MOUSEHOOKSTRUCT* mhs = (MOUSEHOOKSTRUCT*)lParam;
        bool isDoubleClick = (wParam == WM_LBUTTONDBLCLK);
        
        if (wParam == WM_LBUTTONDOWN) {
            ULONGLONG currentTime = GetTickCount64();
            int doubleClickTime = GetDoubleClickTime();
            int sysX = GetSystemMetrics(SM_CXDOUBLECLK);
            int sysY = GetSystemMetrics(SM_CYDOUBLECLK);
            
            if ((currentTime - g_lastClickTime) <= (ULONGLONG)doubleClickTime &&
                abs(mhs->pt.x - g_lastClickPos.x) <= (sysX / 2) &&
                abs(mhs->pt.y - g_lastClickPos.y) <= (sysY / 2)) {
                isDoubleClick = true;
            }
            
            g_lastClickTime = currentTime;
            g_lastClickPos = mhs->pt;
        } else {
            g_lastClickTime = 0; 
        }

        if (isDoubleClick) {
            WCHAR szClass[256];
            GetClassName(mhs->hwnd, szClass, 256);
            
            if (wcscmp(szClass, L"SysListView32") == 0) {
                POINT pt = mhs->pt;
                ScreenToClient(mhs->hwnd, &pt);
                LVHITTESTINFO info;
                ZeroMemory(&info, sizeof(info));
                info.pt = pt;
                int index = SendMessage(mhs->hwnd, LVM_HITTEST, 0, (LPARAM)&info);
                
                if (index == -1) {
                    SendMessage(g_hwndDefView, WM_COMMAND, TOGGLE_ICONS_COMMAND, 0);
                    g_lastClickTime = 0; 
                }
            } 
            else if (wcscmp(szClass, L"SHELLDLL_DefView") == 0 || 
                     wcscmp(szClass, L"WorkerW") == 0 || 
                     wcscmp(szClass, L"Progman") == 0) {
                SendMessage(g_hwndDefView, WM_COMMAND, TOGGLE_ICONS_COMMAND, 0);
                g_lastClickTime = 0; 
            }
        }
    }
    return CallNextHookEx(g_hMouseHook, nCode, wParam, lParam);
}

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    HWND defView = FindWindowEx(hwnd, NULL, L"SHELLDLL_DefView", NULL);
    if (defView) {
        HWND listView = FindWindowEx(defView, NULL, L"SysListView32", L"FolderView");
        if (listView) {
            HWND* ret = (HWND*)lParam;
            *ret = listView;
            g_hwndDefView = defView;
            return FALSE;
        }
    }
    return TRUE;
}

HWND FindDesktopListView() {
    HWND hwndListView = NULL;
    HWND hwndProgman = FindWindow(L"Progman", L"Program Manager");
    HWND defView = FindWindowEx(hwndProgman, NULL, L"SHELLDLL_DefView", NULL);
    if (defView) {
        hwndListView = FindWindowEx(defView, NULL, L"SysListView32", L"FolderView");
        if (hwndListView) {
            g_hwndDefView = defView;
            return hwndListView;
        }
    }
    EnumWindows(EnumWindowsProc, (LPARAM)&hwndListView);
    return hwndListView;
}

BOOL Wh_ModInit() {
    g_hwndDesktopList = FindDesktopListView();
    if (!g_hwndDesktopList || !g_hwndDefView) return FALSE;

    DWORD threadId = GetWindowThreadProcessId(g_hwndDesktopList, NULL);
    if (threadId == 0) return FALSE;

    g_hMouseHook = SetWindowsHookEx(WH_MOUSE, MouseProc, NULL, threadId);
    return (g_hMouseHook != NULL);
}

void Wh_ModUninit() {
    if (g_hMouseHook) {
        UnhookWindowsHookEx(g_hMouseHook);
    }
}