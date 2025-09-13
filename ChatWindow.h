#pragma once
#include <windows.h>
#include <memory>
#include "TextBuffer.h"
#include "Renderer.h"
#include "MessageWindow.h"

class ChatWindow {
public:
    ChatWindow(HINSTANCE hInstance, int width, int height, int margin, std::shared_ptr<TextBuffer> sharedBuffer);

    void Show();
    void Hide();
    void ToggleVisible();

    void SetMessageWindow(MessageWindow* msgWin) { m_textWindow = msgWin; }

private:
    static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
    LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

    void UpdateMessageWindowPosition() const;

    HWND m_hWnd{};
    bool m_visible{false};
    std::shared_ptr<TextBuffer> m_buffer;  // shared buffer safely
    std::unique_ptr<Renderer> m_renderer;
    bool m_destroyed{false};               // window lifetime flag

    MessageWindow* m_textWindow = nullptr;
};
