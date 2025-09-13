#include "ChatWindow.h"

ChatWindow::ChatWindow(HINSTANCE hInstance, int width, int height, int margin) {
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

    // Make black transparent
    SetLayeredWindowAttributes(m_hWnd, RGB(0,0,0), 0, LWA_COLORKEY);

    m_renderer = std::make_unique<Renderer>(m_hWnd);

    // 🔑 Register submit callback
    m_buffer.SetOnSubmit([this](const std::wstring& text) {
        // For now just debug-print the sent message
        OutputDebugStringW((L"Sent message: " + text + L"\n").c_str());

        // Later you can send it to your message list / network here
    });
}

void ChatWindow::Show() {
    m_visible = true;
    ShowWindow(m_hWnd, SW_SHOW);
    SetForegroundWindow(m_hWnd);

    // Start blinking cursor timer (500 ms)
    SetTimer(m_hWnd, 1, 500, nullptr);
}

void ChatWindow::Hide() {
    m_visible = false;
    ShowWindow(m_hWnd, SW_HIDE);

    // Stop timer when hidden
    KillTimer(m_hWnd, 1);
}

void ChatWindow::ToggleVisible() {
    m_visible ? Hide() : Show();
}

LRESULT CALLBACK ChatWindow::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    ChatWindow* self;
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
    switch (msg) {
        case WM_PAINT:
            m_renderer->Paint(m_buffer);
            ValidateRect(m_hWnd, nullptr);
            return 0;

        case WM_CHAR:
            m_buffer.OnChar(wParam);
            InvalidateRect(m_hWnd, nullptr, TRUE);
            return 0;

        case WM_KEYDOWN:
            m_buffer.OnKeyDown(wParam);
            InvalidateRect(m_hWnd, nullptr, TRUE);
            return 0;

        case WM_TIMER:
            m_buffer.OnTimer();
            InvalidateRect(m_hWnd, nullptr, TRUE);
            return 0;

        case WM_KILLFOCUS:
            Hide();
            return 0;

        case WM_DESTROY:
            KillTimer(m_hWnd, 1);
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(m_hWnd, msg, wParam, lParam);
}

