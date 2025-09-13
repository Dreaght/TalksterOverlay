#include "ChatWindow.h"

#include <stdexcept>
#include <utility>

ChatWindow::ChatWindow(HINSTANCE hInstance, int width, int height, int margin, std::shared_ptr<TextBuffer> sharedBuffer)
    : m_buffer(std::move(sharedBuffer))
{
    const char* CLASS_NAME = "ChatOverlay";
    WNDCLASS wc{};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    RegisterClass(&wc);

    RECT screen;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &screen, 0);
    int x = (screen.right - screen.left - width) / 2;
    int y = screen.bottom - height - margin;

    m_hWnd = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW,
        CLASS_NAME, "Chat Overlay", WS_POPUP,
        x, y, width, height, nullptr, nullptr, hInstance, this);

    SetLayeredWindowAttributes(m_hWnd, RGB(0,0,0), 0, LWA_COLORKEY);
    m_renderer = std::make_unique<Renderer>(m_hWnd);

    // Submit callback
    if (m_buffer) {
        std::weak_ptr<TextBuffer> weakBuffer = m_buffer;
        m_buffer->SetOnSubmit([this, weakBuffer](const std::wstring& text) {
            auto buf = weakBuffer.lock();
            if (!buf || text.empty() || m_destroyed) return;
            buf->AddMessage(text);
            if (m_textWindow) m_textWindow->Invalidate();
        });
    }
}

void ChatWindow::UpdateMessageWindowPosition() const {
    RECT chatRect;
    GetWindowRect(m_hWnd, &chatRect);

    int msgWidth  = 600; // same as MessageWindow width
    int msgHeight = 200; // same as MessageWindow height
    int x = chatRect.left + (chatRect.right - chatRect.left - msgWidth) / 2;
    int y = m_visible ? (chatRect.top - msgHeight) : chatRect.top - (msgHeight / 2);

    SetWindowPos(m_textWindow->GetHWND(), HWND_TOPMOST, x, y, msgWidth, msgHeight,
                 SWP_NOACTIVATE | SWP_SHOWWINDOW);
}


void ChatWindow::Show() {
    if (!m_hWnd) return;
    m_visible = true;
    ShowWindow(m_hWnd, SW_SHOW);
    SetForegroundWindow(m_hWnd);
    SetTimer(m_hWnd, 1, 500, nullptr); // cursor blink

    UpdateMessageWindowPosition();
}

void ChatWindow::Hide() {
    if (!m_hWnd) return;
    m_visible = false;
    ShowWindow(m_hWnd, SW_HIDE);
    KillTimer(m_hWnd, 1);

    UpdateMessageWindowPosition();
}

void ChatWindow::ToggleVisible() {
    m_visible ? Hide() : Show();
}

LRESULT CALLBACK ChatWindow::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    ChatWindow* self = nullptr;
    if (msg == WM_NCCREATE) {
        CREATESTRUCT* cs = (CREATESTRUCT*)lParam;
        self = (ChatWindow*)cs->lpCreateParams;
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)self);
        self->m_hWnd = hWnd;
    }
    self = (ChatWindow*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
    return self ? self->HandleMessage(msg, wParam, lParam)
                : DefWindowProc(hWnd, msg, wParam, lParam);
}

LRESULT ChatWindow::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
    if (m_destroyed) return 0;

    switch (msg) {
        case WM_PAINT:
            if (m_renderer && m_buffer) m_renderer->Paint(*m_buffer);
            ValidateRect(m_hWnd, nullptr);
            return 0;

        case WM_CHAR:
            if (m_buffer) m_buffer->OnChar(wParam);
            InvalidateRect(m_hWnd, nullptr, TRUE);
            return 0;

        case WM_KEYDOWN:
            if (m_buffer) m_buffer->OnKeyDown(wParam);
            InvalidateRect(m_hWnd, nullptr, TRUE);
            return 0;

        case WM_TIMER:
            if (m_buffer) m_buffer->OnTimer();
            InvalidateRect(m_hWnd, nullptr, TRUE);
            return 0;

        case WM_KILLFOCUS:
            Hide();
            return 0;

        case WM_DESTROY:
            m_destroyed = true;
            KillTimer(m_hWnd, 1);
            return 0;
    }
    return DefWindowProc(m_hWnd, msg, wParam, lParam);
}
