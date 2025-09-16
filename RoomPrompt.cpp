#include "RoomPrompt.h"
#include "Utils.h"
#include <windows.h>
#include <chrono>
#include <thread>
#include <string>

namespace {

// helper inside cpp file
bool InputBox(const std::wstring& title, const std::wstring& prompt, std::wstring& outText) {
    const int bufSize = 512;
    wchar_t buffer[bufSize] = {};
    if (!outText.empty()) {
        wcsncpy_s(buffer, outText.c_str(), bufSize - 1);
    }
    HWND hWnd = CreateWindowExW(0, L"EDIT", buffer,
                                WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                CW_USEDEFAULT, CW_USEDEFAULT, 400, 100,
                                nullptr, nullptr, GetModuleHandle(nullptr), nullptr);
    if (!hWnd) return false;

    if (MessageBoxW(nullptr, prompt.c_str(), title.c_str(), MB_OKCANCEL | MB_ICONQUESTION) == IDOK) {
        GetWindowTextW(hWnd, buffer, bufSize);
        outText = buffer;
        DestroyWindow(hWnd);
        return true;
    }
    DestroyWindow(hWnd);
    return false;
}

} // namespace

namespace App {
    bool PromptRoomChoice(MatrixClient& matrix) {
        int choice = MessageBoxW(nullptr,
                                 L"Do you want to host a new room?\nYes = Host, No = Join existing room",
                                 L"Room Choice",
                                 MB_YESNO | MB_ICONQUESTION);

        if (choice == IDYES) {
            std::string randomRoomAlias =
                "room_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
            std::string createBody =
                "{\"room_alias_name\":\"" + randomRoomAlias + "\", "
                "\"visibility\":\"public\", "
                "\"preset\":\"private_chat\"}";

            auto resp = matrix.HttpRequest(L"POST", L"/_matrix/client/r0/createRoom", createBody, true);
            std::string roomId = matrix.ExtractJsonValue(resp, "room_id");

            if (!roomId.empty()) {
                bool joined = false;
                const int maxRetries = 20;
                const int delayMs = 500;
                for (int i = 0; i < maxRetries; ++i) {
                    joined = matrix.JoinRoom(roomId);
                    if (joined) break;
                    std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
                }

                if (joined) {
                    MessageBoxW(nullptr,
                                (L"Room created! Link: #" + ToWString(randomRoomAlias) + L":matrix.org").c_str(),
                                L"Room Info", MB_OK | MB_ICONINFORMATION);
                    return true;
                } else {
                    MessageBoxW(nullptr, L"Failed to join the newly created room!", L"Error", MB_ICONERROR);
                    return false;
                }
            } else {
                MessageBoxW(nullptr, L"Failed to create room!", L"Error", MB_ICONERROR);
                return false;
            }
        } else {
            std::wstring roomLink;
            if (auto lastRoom = MatrixClient::LoadLastRoomLink()) {
                roomLink = ToWString(*lastRoom);
            }
            if (InputBox(L"Join Room", L"Enter room link (or leave as suggested):", roomLink) && !roomLink.empty()) {
                if (matrix.JoinRoom(ToString(roomLink))) {
                    MessageBoxW(nullptr, L"Joined room successfully!", L"Info", MB_OK | MB_ICONINFORMATION);
                    return true;
                } else {
                    MessageBoxW(nullptr, L"Failed to join room!", L"Error", MB_ICONERROR);
                    return PromptRoomChoice(matrix);
                }
            } else {
                return PromptRoomChoice(matrix);
            }
        }
    }
}
