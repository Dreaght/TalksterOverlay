#include "client/WebSocketClient.h"
#include "window/ChatWindow.h"
#include "window/MessageWindow.h"
#include "HotkeyManager.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    auto sharedBuffer = std::make_shared<TextBuffer>();

    MessageWindow messages(hInstance, 600, 200, 500, 500, sharedBuffer);
    ChatWindow chat(hInstance, 600, 80, 50, sharedBuffer);

    chat.SetMessageWindow(&messages);


    // auto wsClient = std::make_shared<WebSocketClient>("127.0.0.1", 9001);
    // wsClient->SetOnMessage([&](const std::wstring& msg) {
    //     chat.OnExternalMessage(msg, false); // false = received
    // });
    // wsClient->Connect();
    //
    // sharedBuffer->AddOnSubmitHandler([&](const std::wstring& text) {
    //     wsClient->Send(text);
    // });


    HotkeyManager hotkeys;
    hotkeys.Register(1, MOD_ALT, 'X'); // toggle chat
    hotkeys.Register(2, MOD_ALT, 'Q'); // quit
    hotkeys.Register(3, 0, VK_ESCAPE); // hide chat

    chat.Show();
    messages.Show();

    MSG msg{};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        if (msg.message == WM_HOTKEY) {
            switch (msg.wParam) {
                case 1: chat.ToggleVisible(); break;
                case 2: PostQuitMessage(0); break;
                case 3: chat.Hide(); break;
            }
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}
