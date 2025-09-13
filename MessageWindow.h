#pragma once
#include <windows.h>
#include <memory>

#include "MessageRenderer.h"
#include "Renderer.h"
#include "TextBuffer.h"

class MessageWindow {
public:
    MessageWindow(HINSTANCE hInstance, int x, int y, int width, int height,
                  std::shared_ptr<TextBuffer> sharedBuffer);
    ~MessageWindow();

    HWND GetHwnd() const { return m_hWnd; }

    void Show();
    void Invalidate();

    HWND GetHWND() const { return m_hWnd; }


private:
    HWND m_hWnd{};
    std::unique_ptr<MessageRenderer> m_renderer;
    std::shared_ptr<TextBuffer> m_buffer;  // shared with ChatWindow

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);
};
