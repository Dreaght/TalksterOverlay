#pragma once
#include <string>
#include <vector>
#include <windows.h>
#include <functional>

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
    void AddMessage(const std::wstring& msg) { m_messages.push_back(msg); }
    void RemoveMessage(size_t index) {
        if (index < m_messages.size())
            m_messages.erase(m_messages.begin() + index);
    }
    const std::vector<std::wstring>& GetMessages() const { return m_messages; }
    void ClearMessages() { m_messages.clear(); }

    // Submit callback
    std::function<void(const std::wstring&)> m_onSubmit;
    void SetOnSubmit(std::function<void(const std::wstring&)> cb) { m_onSubmit = std::move(cb); }

private:
    std::wstring m_text;
    size_t m_cursorPos{0};
    bool m_cursorVisible{true};
    DWORD m_lastInput{0};
    const DWORD m_blinkDelay{500};

    // Temporary messages
    std::vector<std::wstring> m_messages;
};
