#include "TextBuffer.h"

void TextBuffer::OnChar(WPARAM wParam) {
    wchar_t ch = static_cast<wchar_t>(wParam);
    const size_t maxLength = 165; // set your limit here

    if (ch >= 32 && ch != 127) { // printable
        if (m_text.size() < maxLength) { // only insert if under limit
            m_text.insert(m_cursorPos, 1, ch);
            m_cursorPos++;
        }
        m_cursorVisible = true;
        m_lastInput = GetTickCount();
    }
}

void TextBuffer::OnKeyDown(WPARAM wParam) {
    switch (wParam) {
        case VK_BACK:
            if (m_cursorPos > 0) {
                m_text.erase(m_cursorPos - 1, 1);
                m_cursorPos--;
            }
            break;
        case VK_DELETE:
            if (m_cursorPos < m_text.size()) {
                m_text.erase(m_cursorPos, 1);
            }
            break;
        case VK_LEFT:
            if (m_cursorPos > 0) m_cursorPos--;
            break;
        case VK_RIGHT:
            if (m_cursorPos < m_text.size()) m_cursorPos++;
            break;
        case VK_HOME:
            m_cursorPos = 0;
            break;
        case VK_END:
            m_cursorPos = m_text.size();
            break;
        case VK_RETURN:
        {
            // Store the text somewhere (e.g. pass to callback)
            if (m_onSubmit) {
                m_onSubmit(m_text);  // call callback with current text
            }

            // Clear text after sending
            m_text.clear();
            m_cursorPos = 0;
            m_cursorVisible = true;
            m_lastInput = GetTickCount();
            break;
        }
    }
    m_cursorVisible = true;
    m_lastInput = GetTickCount();
}

void TextBuffer::OnTimer() {
    DWORD now = GetTickCount();
    if (now - m_lastInput >= m_blinkDelay) {
        m_cursorVisible = !m_cursorVisible;
        m_lastInput = now;
    }
}
