#pragma once
#include <windows.h>

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

