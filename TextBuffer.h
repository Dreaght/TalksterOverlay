#pragma once
#include <string>
#include <windows.h>
#include <functional>

class TextBuffer {
public:
    void OnChar(WPARAM wParam);
    void OnKeyDown(WPARAM wParam);
    void OnTimer();

    const std::wstring& GetText() const { return m_text; }
    size_t GetCursorPos() const { return m_cursorPos; }
    bool IsCursorVisible() const { return m_cursorVisible; }
    std::function<void(const std::wstring&)> m_onSubmit;

    void SetOnSubmit(std::function<void(const std::wstring&)> cb) {
        m_onSubmit = std::move(cb);
    }

private:
    std::wstring m_text;
    size_t m_cursorPos{0};
    bool m_cursorVisible{true};
    DWORD m_lastInput{0};
    const DWORD m_blinkDelay{500};
};
