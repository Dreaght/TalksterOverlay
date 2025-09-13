#pragma once
#include <d2d1.h>
#include <dwrite.h>
#include <string>
#include "TextBuffer.h"

class Renderer {
public:
    explicit Renderer(HWND hWnd);
    ~Renderer();
    void Paint(const TextBuffer& buffer);

private:
    HWND m_hWnd{};
    ID2D1Factory* m_factory{};
    ID2D1HwndRenderTarget* m_target{};
    ID2D1SolidColorBrush* m_brush{};
    IDWriteFactory* m_dwrite{};
    IDWriteTextFormat* m_format{};

    void CreateResources();
    void DiscardResources();
};
