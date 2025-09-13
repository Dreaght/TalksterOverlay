#pragma once
#include <windows.h>
#include <memory>
#include "Renderer.h"
#include "TextBuffer.h"

class ChatWindow {
public:
    ChatWindow(HINSTANCE hInstance, int width, int height, int margin);
    void Show();
    void Hide();
    void ToggleVisible();
    HWND GetHandle() const { return m_hWnd; }

private:
    static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
    LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

    HWND m_hWnd{};
    bool m_visible{false};
    std::unique_ptr<Renderer> m_renderer;
    TextBuffer m_buffer;
};
