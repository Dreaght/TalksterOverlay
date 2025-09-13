#include "MessageWindow.h"
#include <stdexcept>

MessageWindow::MessageWindow(HINSTANCE hInstance, int x, int y, int width, int height,
                             std::shared_ptr<TextBuffer> sharedBuffer)
    : m_buffer(std::move(sharedBuffer))
{
    const char* CLASS_NAME = "MessageOverlayWindow";

    WNDCLASS wc{};
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.cbWndExtra    = sizeof(LONG_PTR);

    RegisterClass(&wc);

    m_hWnd = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW,
        CLASS_NAME,
        "Message Overlay",
        WS_POPUP,
        x, y, width, height,
        nullptr, nullptr,
        hInstance,
        this
    );

    if (!m_hWnd) {
        throw std::runtime_error("Failed to create MessageWindow");
    }

    m_renderer = std::make_unique<MessageRenderer>(m_hWnd);

    SetLayeredWindowAttributes(m_hWnd, RGB(0,0,0), 0, LWA_COLORKEY);
}

MessageWindow::~MessageWindow() {
    if (m_hWnd) {
        DestroyWindow(m_hWnd);
    }
}

void MessageWindow::Show() {
    if (m_hWnd) {
        ShowWindow(m_hWnd, SW_SHOW);
        UpdateWindow(m_hWnd);
    }
}

void MessageWindow::Invalidate() {
    if (m_hWnd) {
        InvalidateRect(m_hWnd, nullptr, TRUE);
    }
}

LRESULT CALLBACK MessageWindow::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    MessageWindow* self = nullptr;
    if (msg == WM_NCCREATE) {
        CREATESTRUCT* cs = (CREATESTRUCT*)lParam;
        self = (MessageWindow*)cs->lpCreateParams;
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)self);
        self->m_hWnd = hWnd;
    }
    self = (MessageWindow*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
    return self ? self->HandleMessage(msg, wParam, lParam)
                : DefWindowProc(hWnd, msg, wParam, lParam);
}

LRESULT MessageWindow::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            BeginPaint(m_hWnd, &ps);

            if (m_renderer && m_buffer) {
                m_renderer->Paint(*m_buffer);
            }

            EndPaint(m_hWnd, &ps);
            return 0;
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(m_hWnd, msg, wParam, lParam);
}
