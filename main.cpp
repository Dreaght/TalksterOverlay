#include "client/MatrixClient.h"
#include "window/ChatWindow.h"
#include "window/MessageWindow.h"
#include "HotkeyManager.h"
#include <memory>
#include <string>
#include <windows.h>

// -------------------- Helpers --------------------
std::wstring ToWString(const std::string& s) {
    return std::wstring(s.begin(), s.end());
}

std::string ToString(const std::wstring& ws) {
    return std::string(ws.begin(), ws.end());
}

// -------------------- Matrix Setup --------------------
bool SetupMatrix(MatrixClient& matrix, ChatWindow& chat) {
    // Use SSO + auto-create random room
    if (!matrix.LoginWithSSOAndRandomRoomAsync().get()) {
        return false; // Login failed, error already shown in MatrixClient
    }

    // Set callback for incoming messages
    matrix.SetOnMessage([&](const std::string& roomId, const std::string& msg) {
        chat.OnExternalMessage(ToWString(msg), false);
    });

    matrix.Start(); // start async long-polling
    return true;
}

// -------------------- GUI Setup --------------------
void SetupWindows(HINSTANCE hInstance,
                  std::shared_ptr<TextBuffer>& sharedBuffer,
                  MessageWindow*& messagesOut,
                  ChatWindow*& chatOut) {
    messagesOut = new MessageWindow(hInstance, 600, 200, 500, 500, sharedBuffer);
    chatOut = new ChatWindow(hInstance, 600, 80, 50, sharedBuffer);

    chatOut->SetMessageWindow(messagesOut);
    messagesOut->Show();
    chatOut->Show();
}

// -------------------- Hotkeys --------------------
void SetupHotkeys(HotkeyManager& hotkeys) {
    hotkeys.Register(1, MOD_ALT, 'X'); // toggle chat
    hotkeys.Register(2, MOD_ALT, 'Q'); // quit
    hotkeys.Register(3, 0, VK_ESCAPE); // hide chat
}

// -------------------- Message Sending --------------------
void SetupMessageSending(std::shared_ptr<TextBuffer>& sharedBuffer, MatrixClient& matrix) {
    sharedBuffer->AddOnSubmitHandler([&](const std::wstring& text) {
        std::string msg(ToString(text));
        std::thread([&, msg]() {
            matrix.SendMessageAsync("#randomroom:matrix.org", msg);
        }).detach();
    });
}

// -------------------- WinMain --------------------
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    auto sharedBuffer = std::make_shared<TextBuffer>();

    MessageWindow* messages = nullptr;
    ChatWindow* chat = nullptr;
    SetupWindows(hInstance, sharedBuffer, messages, chat);

    // MatrixClient matrix(L"matrix.org");
    //
    // // Start async SSO login without blocking GUI
    // auto loginFuture = matrix.LoginWithSSOAndRandomRoomAsync();
    //
    // while (loginFuture.wait_for(std::chrono::milliseconds(100)) != std::future_status::ready) {
    //     MSG msg{};
    //     while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
    //         TranslateMessage(&msg);
    //         DispatchMessage(&msg);
    //     }
    // }
    //
    // // Check login result
    // if (!loginFuture.get()) return 1;
    //
    // // Continue with message sending + hotkeys
    // SetupMessageSending(sharedBuffer, matrix);


    HotkeyManager hotkeys;
    SetupHotkeys(hotkeys);

    // -------------------- Main Message Loop --------------------
    MSG msg{};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        if (msg.message == WM_HOTKEY) {
            switch (msg.wParam) {
                case 1: chat->ToggleVisible(); break;
                case 2: PostQuitMessage(0); break;
                case 3: chat->Hide(); break;
            }
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Cleanup
    // matrix.Stop();
    delete messages;
    delete chat;

    return 0;
}
