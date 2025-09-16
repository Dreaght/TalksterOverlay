#include "client/MatrixClient.h"
#include "window/ChatWindow.h"
#include "window/MessageWindow.h"
#include "HotkeyManager.h"

#include "HotkeySetup.h"
#include "MessageSending.h"
#include "RoomPrompt.h"

#include <windows.h>
#include <memory>
#include <future>

#include "client/MatrixSetup.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    auto sharedBuffer = std::make_shared<TextBuffer>();

    MessageWindow messages(hInstance, 450, 200, 750, 600, sharedBuffer);
    ChatWindow chat(hInstance, 600, 80, 50, sharedBuffer);
    chat.SetMessageWindow(&messages);

    MatrixClient matrix(L"matrix.org");
    matrix.SetChatWindowHandle(chat.GetHWND());

    matrix.SetOnLogin([&](bool success) {
        if (!success) {
            MessageBox(nullptr, "Login failed", "Error", MB_ICONERROR);
            PostQuitMessage(1);
            return;
        }

        App::SetupMatrix(matrix, chat);
        App::SetupMessageSending(sharedBuffer, matrix);

        if (!App::PromptRoomChoice(matrix)) {
            PostQuitMessage(1);
        }
    });

    auto loginFuture = std::async(std::launch::async, [&matrix]() {
        return matrix.LoginWithSSOAsync().get();
    });

    HotkeyManager hotkeys;
    App::SetupHotkeys(hotkeys);

    chat.Show();
    messages.Show();

    MSG msg{};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        if (msg.message == WM_HOTKEY) {
            switch (msg.wParam) {
                case 1: chat.ToggleVisible(); break;
                case 2: matrix.Stop(); PostQuitMessage(0); break;
                case 3: chat.Hide(); break;
            }
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}
