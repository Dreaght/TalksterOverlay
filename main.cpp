#include "ChatWindow.h"
#include "HotkeyManager.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    ChatWindow window(hInstance, 600, 80, 50);
    HotkeyManager hotkeys;

    hotkeys.Register(1, MOD_ALT, 'X'); // toggle
    hotkeys.Register(2, MOD_ALT, 'Q'); // quit
    hotkeys.Register(3, 0, VK_ESCAPE); // hide

    window.Show();

    MSG msg{};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        if (msg.message == WM_HOTKEY) {
            switch (msg.wParam) {
                case 1: window.ToggleVisible(); break;
                case 2: PostQuitMessage(0); break;
                case 3: window.Hide(); break;
            }
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}
