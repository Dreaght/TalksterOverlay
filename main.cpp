#include "client/MatrixClient.h"
#include "window/ChatWindow.h"
#include "window/MessageWindow.h"
#include "HotkeyManager.h"
#include "Utils.h"
#include <memory>
#include <string>
#include <windows.h>
#include <thread>
#include <chrono>

#define WM_MATRIX_LOGIN_COMPLETE (WM_APP + 1)
#define WM_MATRIX_MESSAGE (WM_APP + 100)

// -------------------- Matrix Setup --------------------
void SetupMatrix(MatrixClient& matrix, ChatWindow& chat) {
    matrix.SetOnMessage([&](const std::string& roomId, const std::string& msg) {
        // MessageBoxW(nullptr, ToWString(msg).c_str(), L"Info", MB_OK | MB_ICONINFORMATION);
        chat.OnExternalMessage(ToWString(msg), false);
    });
    matrix.Start();
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

// -------------------- Ask user for room choice --------------------
// Simple InputBox using wide strings
bool InputBox(const std::wstring& title, const std::wstring& prompt, std::wstring& outText) {
    const int bufSize = 512;
    wchar_t buffer[bufSize] = {};

    // Create a basic edit control in a temporary window
    HWND hWnd = CreateWindowExW(0, L"EDIT", L"",  // <-- Use L"EDIT"
                                WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                CW_USEDEFAULT, CW_USEDEFAULT, 400, 100,
                                nullptr, nullptr, GetModuleHandle(nullptr), nullptr);
    if (!hWnd) return false;

    if (MessageBoxW(nullptr, prompt.c_str(), title.c_str(), MB_OKCANCEL | MB_ICONQUESTION) == IDOK) { // <-- MessageBoxW
        GetWindowTextW(hWnd, buffer, bufSize); // <-- GetWindowTextW
        outText = buffer;
        DestroyWindow(hWnd);
        return true;
    }
    DestroyWindow(hWnd);
    return false;
}

bool PromptRoomChoice(MatrixClient& matrix) {
    int choice = MessageBoxW(nullptr,  // <-- Use MessageBoxW
                             L"Do you want to host a new room?\nYes = Host, No = Join existing room",
                             L"Room Choice",
                             MB_YESNO | MB_ICONQUESTION);

    bool roomJoined = false;
    if (choice == IDYES) {
        std::string randomRoomAlias =
            "room_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        std::string createBody =
            "{\"room_alias_name\":\"" + randomRoomAlias + "\", \"visibility\":\"private\"}";
        auto resp = matrix.HttpRequest(L"POST", L"/_matrix/client/r0/createRoom", createBody, true);
        std::string roomId = matrix.ExtractJsonValue(resp, "room_id");

        if (!roomId.empty()) {
            roomJoined = matrix.JoinRoom(roomId);
            MessageBoxW(nullptr,
                        (L"Room created! Link: #" + ToWString(randomRoomAlias) + L":matrix.org").c_str(),
                        L"Room Info", MB_OK | MB_ICONINFORMATION);
        } else {
            MessageBoxW(nullptr, L"Failed to create room!", L"Error", MB_ICONERROR);
        }
    } else {
        std::wstring roomLink;
        if (InputBox(L"Join Room", L"Enter room link:", roomLink) && !roomLink.empty()) {
            roomJoined = matrix.JoinRoom(ToString(roomLink));
            if (roomJoined) {
                MessageBoxW(nullptr, L"Joined room successfully!", L"Info", MB_OK | MB_ICONINFORMATION);
            } else {
                MessageBoxW(nullptr, L"Failed to join room!", L"Error", MB_ICONERROR);
                PromptRoomChoice(matrix);
            }
        } else {
            PromptRoomChoice(matrix);
        }
    }

    return roomJoined;
}

// -------------------- WinMain --------------------
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

        MessageBox(nullptr, "Login successful!", "Success", MB_OK);
        SetupMatrix(matrix, chat);
        SetupMessageSending(sharedBuffer, matrix);

        // Ask user whether to host or join
        if (!PromptRoomChoice(matrix)) {
            PostQuitMessage(1);
        }
    });

    // Only login; room choice handled after login
    std::thread([&matrix]() {
        matrix.LoginWithSSOAsync().get(); // blocking for simplicity
    }).detach();

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
