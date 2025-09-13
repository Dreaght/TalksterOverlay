#include "Renderer.h"
#include <stdexcept>

Renderer::Renderer(HWND hWnd) : m_hWnd(hWnd) {
    // Init D2D factory
    if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_factory))) {
        throw std::runtime_error("Failed to create D2D factory");
    }
    // Init DWrite
    if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
                                   __uuidof(IDWriteFactory),
                                   reinterpret_cast<IUnknown**>(&m_dwrite)))) {
        throw std::runtime_error("Failed to create DWrite factory");
    }
    if (FAILED(m_dwrite->CreateTextFormat(
                   L"Segoe UI", nullptr,
                   DWRITE_FONT_WEIGHT_REGULAR,
                   DWRITE_FONT_STYLE_NORMAL,
                   DWRITE_FONT_STRETCH_NORMAL,
                   20.0f, L"en-us", &m_format))) {
        throw std::runtime_error("Failed to create text format");
    }
    m_format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    m_format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    CreateResources();
}

Renderer::~Renderer() {
    DiscardResources();
    if (m_format) m_format->Release();
    if (m_dwrite) m_dwrite->Release();
    if (m_factory) m_factory->Release();
}

void Renderer::CreateResources() {
    if (m_target) return;

    RECT rc;
    GetClientRect(m_hWnd, &rc);
    D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);

    D2D1_RENDER_TARGET_PROPERTIES rtProps = D2D1::RenderTargetProperties();
    D2D1_HWND_RENDER_TARGET_PROPERTIES hwndProps =
        D2D1::HwndRenderTargetProperties(m_hWnd, size);

    m_factory->CreateHwndRenderTarget(rtProps, hwndProps, &m_target);
    m_target->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &m_brush);
}

void Renderer::DiscardResources() {
    if (m_brush) { m_brush->Release(); m_brush = nullptr; }
    if (m_target) { m_target->Release(); m_target = nullptr; }
}

void Renderer::Paint(const TextBuffer& buffer) {
    CreateResources();
    m_target->BeginDraw();
    m_target->Clear(D2D1::ColorF(0, 0)); // transparent

    RECT rc;
    GetClientRect(m_hWnd, &rc);
    D2D1_RECT_F layout = D2D1::RectF(
        static_cast<FLOAT>(rc.left), static_cast<FLOAT>(rc.top),
        static_cast<FLOAT>(rc.right), static_cast<FLOAT>(rc.bottom));

    std::wstring text = buffer.GetText();
    if (buffer.IsCursorVisible()) {
        text.insert(buffer.GetCursorPos(), L"|");
    }

    // Drop shadow
    ID2D1SolidColorBrush* shadowBrush = nullptr;
    m_target->CreateSolidColorBrush(D2D1::ColorF(0.1f, 0.1f, 0.1f, 0.5f), &shadowBrush);

    const float shadowOffsets[] = { -1.5f, 0.0f, 1.5f };
    for (float dx : shadowOffsets) {
        for (float dy : shadowOffsets) {
            D2D1_RECT_F shadowRect = D2D1::RectF(
                layout.left + dx, layout.top + dy,
                layout.right + dx, layout.bottom + dy);
            m_target->DrawText(
            text.c_str(),
                static_cast<UINT32>(text.length()),
                m_format,
                shadowRect,
                shadowBrush
            );
        }
    }

    // Actual text
    m_brush->SetColor(D2D1::ColorF(D2D1::ColorF::White));
    m_target->DrawText(
    text.c_str(),
        static_cast<UINT32>(text.length()),
        m_format,
        layout,
        m_brush
    );

    if (shadowBrush) shadowBrush->Release();

    HRESULT hr = m_target->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET) {
        DiscardResources();
    }
}
