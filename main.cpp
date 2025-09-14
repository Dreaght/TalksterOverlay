#include "client/MatrixClient.h"
#include "window/ChatWindow.h"
#include "window/MessageWindow.h"
#include "HotkeyManager.h"
#include <memory>
#include <string>
#include <windows.h>

#define WM_MATRIX_LOGIN_COMPLETE (WM_APP + 1)

// -------------------- Helpers --------------------
std::wstring ToWString(const std::string& s) {
    return std::wstring(s.begin(), s.end());
}

std::string ToString(const std::wstring& ws) {
    return std::string(ws.begin(), ws.end());
}

// -------------------- Matrix Setup --------------------
void SetupMatrix(MatrixClient& matrix, ChatWindow& chat) {
    // Set callback for incoming messages
    matrix.SetOnMessage([&](const std::string& roomId, const std::string& msg) {
        chat.OnExternalMessage(ToWString(msg), false);
    });

    matrix.Start(); // start async long-polling
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
            if (!matrix.m_currentRoomId.empty()) {
                matrix.SendMessageAsync(matrix.m_currentRoomId, msg);
            } else {
                MessageBox(nullptr, "No room joined yet!", "Error", MB_ICONERROR);
            }
        }).detach();
    });
}


// -------------------- WinMain --------------------
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    auto sharedBuffer = std::make_shared<TextBuffer>();

    MessageWindow messages(hInstance, 500, 200, 600, 500, sharedBuffer);
    ChatWindow chat(hInstance, 600, 80, 50, sharedBuffer);
    chat.SetMessageWindow(&messages);

    MatrixClient matrix(L"matrix.org");

    matrix.SetOnLogin([&](bool success) {
        if (success) {
            MessageBox(nullptr, "Setting up matrix...", "Success", MB_OK);
            SetupMatrix(matrix, chat);
            SetupMessageSending(sharedBuffer, matrix);
        } else {
            MessageBox(nullptr, "Login failed", "Error", MB_ICONERROR);
            PostQuitMessage(1);
        }
    });

    matrix.LoginWithSSOAndRandomRoomAsync();

    HotkeyManager hotkeys;
    SetupHotkeys(hotkeys);

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

