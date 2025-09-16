#include "MessageSending.h"
#include "Utils.h"
#include <future>
#include <windows.h>

namespace App {
    void SetupMessageSending(std::shared_ptr<TextBuffer>& sharedBuffer, MatrixClient& matrix) {
        sharedBuffer->AddOnSubmitHandler([&](const std::wstring& text) {
            std::string msg(ToString(text));
            std::async(std::launch::async, [&matrix, msg]() {
                if (!matrix.m_currentRoomId.empty()) {
                    matrix.SendMessageAsync(matrix.m_currentRoomId, msg);
                } else {
                    MessageBox(nullptr, "No room joined yet!", "Error", MB_ICONERROR);
                }
            });
        });
    }
}
