#pragma once
#include <string>
#include <vector>
#include <windows.h>
#include <functional>

struct TimedMessage {
    std::wstring text;
    DWORD timestamp;   // when added
    float alpha;       // 0..1
    DWORD fullVisible; // ms fully opaque
    DWORD fadeOut;     // ms fade duration
    bool sent;         // true = sent by me, false = received
};

class TextBuffer {
public:
    void OnChar(WPARAM wParam);
    void OnKeyDown(WPARAM wParam);
    void OnTimer();

    // Current input text
    const std::wstring& GetText() const { return m_text; }
    size_t GetCursorPos() const { return m_cursorPos; }
    bool IsCursorVisible() const { return m_cursorVisible; }

    // Messages management
    void AddMessage(const std::wstring &msg, bool sent);
    const std::vector<TimedMessage>& GetMessages() const { return m_messages; }
    void ClearMessages() { m_messages.clear(); }

    // Submit callback
    std::vector<std::function<void(const std::wstring&)>> m_submitHandlers;
    void AddOnSubmitHandler(std::function<void(const std::wstring&)> cb);

private:
    std::wstring m_text;
    size_t m_cursorPos{0};
    bool m_cursorVisible{true};
    DWORD m_lastInput{0};
    const DWORD m_blinkDelay{500};

    // Temporary messages
    std::vector<TimedMessage> m_messages;
};
