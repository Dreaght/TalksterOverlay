#pragma once
#include <windows.h>

#define ID_TRAY_QUIT 1001
#define ID_TRAY_EDIT_BINDINGS 1002

class TrayIcon {
public:
    TrayIcon(HINSTANCE hInstance, HWND target);
    ~TrayIcon();
    void ShowMenu(POINT pt);

    HWND targetHwnd;
private:
    NOTIFYICONDATAA nid{};
    HMENU hMenu{};
    HWND hWnd{};
};

