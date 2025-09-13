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
            if (m_onSubmit && !m_text.empty()) {
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

    // Blink cursor
    if (now - m_lastInput >= m_blinkDelay) {
        m_cursorVisible = !m_cursorVisible;
        m_lastInput = now;
    }

    for (auto it = m_messages.begin(); it != m_messages.end();) {
        DWORD elapsed = now - it->timestamp;
        DWORD totalLife = it->fullVisible + it->fadeOut;

        if (elapsed >= totalLife) {
            it = m_messages.erase(it);
        } else {
            if (elapsed < it->fullVisible) {
                it->alpha = 1.0f;
            } else {
                float t = float(elapsed - it->fullVisible) / float(it->fadeOut);
                it->alpha = 1.0f - t;
            }
            ++it;
        }
    }
}

void TextBuffer::AddMessage(const std::wstring& msg, bool sent) {
    TimedMessage m;
    m.text = msg;
    m.timestamp = GetTickCount();
    m.alpha = 1.0f;
    m.sent = sent;

    // --- lifetime scaling ---
    const DWORD baseFullVisible = 1000; // minimum ms fully visible
    const DWORD perCharExtra    = 50;   // add ms per character
    const DWORD maxFullVisible  = 5000; // cap
    const DWORD fadeOut         = 2000; // always 2s fade

    DWORD visibleTime = baseFullVisible + (DWORD)msg.size() * perCharExtra;
    if (visibleTime > maxFullVisible) visibleTime = maxFullVisible;

    m.fullVisible = visibleTime;
    m.fadeOut     = fadeOut;

    m_messages.push_back(std::move(m));
}
