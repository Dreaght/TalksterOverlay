#include "TrayIcon.h"
#include "../resource.h"

#define WM_TRAYICON (WM_USER + 1)

static LRESULT CALLBACK TrayWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    TrayIcon* self = reinterpret_cast<TrayIcon*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    if (msg == WM_TRAYICON && lParam == WM_RBUTTONUP) {
        POINT pt;
        GetCursorPos(&pt);
        if (self && self->targetHwnd) {
            PostMessage(self->targetHwnd, WM_APP + 2, pt.x, pt.y);
        }
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

TrayIcon::TrayIcon(HINSTANCE hInstance, HWND target) : targetHwnd(target) {
    WNDCLASSA wc{};
    wc.lpfnWndProc = TrayWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "TrayIconHiddenWindow";
    RegisterClassA(&wc);

    hWnd = CreateWindowA("TrayIconHiddenWindow", nullptr, 0, 0, 0, 0, 0, nullptr, nullptr, hInstance, nullptr);
    SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

    hMenu = CreatePopupMenu();
    AppendMenuA(hMenu, MF_STRING, ID_TRAY_EDIT_BINDINGS, "Edit Key Bindings");
    AppendMenuA(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuA(hMenu, MF_STRING, ID_TRAY_QUIT, "Quit  -  <ALT + Q>");

    nid.cbSize = sizeof(NOTIFYICONDATAA);
    nid.hWnd = hWnd;
    nid.uID = 1;
    nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = (HICON)LoadImage(
        hInstance,
        MAKEINTRESOURCE(IDI_TRAY_ICON),
        IMAGE_ICON,
        0, 0,
        LR_DEFAULTSIZE | LR_SHARED
    );

    strcpy_s(nid.szTip, "Chat Overlay");
    Shell_NotifyIconA(NIM_ADD, &nid);
}

TrayIcon::~TrayIcon() {
    Shell_NotifyIconA(NIM_DELETE, &nid);
    DestroyMenu(hMenu);
    DestroyWindow(hWnd);
}

void TrayIcon::ShowMenu(POINT pt) {
    SetForegroundWindow(hWnd);
    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, nullptr);
}
