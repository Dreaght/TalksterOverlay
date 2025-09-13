#include "MessageRenderer.h"
#include <stdexcept>
#include <iostream>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

MessageRenderer::MessageRenderer(HWND hWnd) : m_hWnd(hWnd) {
    if (!m_hWnd) throw std::runtime_error("Invalid HWND");

    if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_factory)) || !m_factory)
        throw std::runtime_error("Failed to create D2D factory");

    if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
                                   __uuidof(IDWriteFactory),
                                   reinterpret_cast<IUnknown**>(&m_dwrite))) || !m_dwrite)
        throw std::runtime_error("Failed to create DWrite factory");

    if (FAILED(m_dwrite->CreateTextFormat(
            L"Segoe UI", nullptr,
            DWRITE_FONT_WEIGHT_REGULAR,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            25.0f, L"en-us", &m_format)) || !m_format)
        throw std::runtime_error("Failed to create text format");

    m_format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    m_format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
}

MessageRenderer::~MessageRenderer() {
    DiscardResources();
    if (m_format) m_format->Release();
    if (m_dwrite) m_dwrite->Release();
    if (m_factory) m_factory->Release();
}

void MessageRenderer::CreateResources() {
    if (m_target) return; // already created

    RECT rc;
    if (!GetClientRect(m_hWnd, &rc)) throw std::runtime_error("Failed to get client rect");

    D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);
    D2D1_RENDER_TARGET_PROPERTIES rtProps = D2D1::RenderTargetProperties();
    D2D1_HWND_RENDER_TARGET_PROPERTIES hwndProps = D2D1::HwndRenderTargetProperties(m_hWnd, size);

    HRESULT hr = m_factory->CreateHwndRenderTarget(rtProps, hwndProps, &m_target);
    if (FAILED(hr) || !m_target) throw std::runtime_error("Failed to create HWND render target");

    hr = m_target->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &m_brush);
    if (FAILED(hr) || !m_brush) throw std::runtime_error("Failed to create brush");
}

void MessageRenderer::DiscardResources() {
    if (m_brush) { m_brush->Release(); m_brush = nullptr; }
    if (m_target) { m_target->Release(); m_target = nullptr; }
}

void MessageRenderer::Paint(const TextBuffer& buffer) {
    CreateResources();

    m_target->BeginDraw();

    // Transparent background
    m_target->Clear(D2D1::ColorF(0, 0));

    if (m_brush && m_format) {
        const auto& messages = buffer.GetMessages();

        RECT rc;
        GetClientRect(m_hWnd, &rc);
        float clientWidth  = static_cast<float>(rc.right - rc.left);
        float clientHeight = static_cast<float>(rc.bottom - rc.top);

        // Compute line height
        ComPtr<IDWriteTextLayout> textLayout;
        if (FAILED(m_dwrite->CreateTextLayout(
                L"A", 1, m_format,
                clientWidth, clientHeight,
                &textLayout))) {
            throw std::runtime_error("Failed to create text layout for metrics");
        }
        DWRITE_TEXT_METRICS metrics{};
        textLayout->GetMetrics(&metrics);
        float lineHeight = metrics.height;

        // Start from bottom
        float y = clientHeight;

        // Create shadow brush
        ID2D1SolidColorBrush* shadowBrush = nullptr;
        m_target->CreateSolidColorBrush(D2D1::ColorF(0.1f, 0.1f, 0.1f, 0.5f), &shadowBrush);

        const float shadowOffsets[] = { -1.5f, 0.0f, 1.5f };

        // Draw messages bottom-to-top
        for (auto it = messages.rbegin(); it != messages.rend(); ++it) {
            const TimedMessage& msg = *it;

            ComPtr<IDWriteTextLayout> layout;
            HRESULT hr = m_dwrite->CreateTextLayout(
                msg.text.c_str(),
                static_cast<UINT32>(msg.text.length()),
                m_format,
                clientWidth,
                clientHeight,
                &layout
            );
            if (FAILED(hr) || !layout) continue;

            DWRITE_TEXT_METRICS tm{};
            layout->GetMetrics(&tm);

            y -= tm.height;
            if (y < 0) break;

            // Center align
            layout->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);

            // --- Draw shadow first ---
            for (float dx : shadowOffsets) {
                for (float dy : shadowOffsets) {
                    m_target->DrawTextLayout(
                        D2D1::Point2F(dx, y + dy),
                        layout.Get(),
                        shadowBrush
                    );
                }
            }

            // --- Draw actual message text ---
            m_brush->SetColor(D2D1::ColorF(D2D1::ColorF::White, msg.alpha));
            m_target->DrawTextLayout(
                D2D1::Point2F(0.0f, y),
                layout.Get(),
                m_brush
            );
        }

        if (shadowBrush) shadowBrush->Release();

        // Reset alignment
        m_format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    }

    HRESULT hr = m_target->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET) {
        DiscardResources();
    }
}

