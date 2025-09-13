#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <string>
#pragma comment(lib, "d2d1")
#pragma comment(lib, "dwrite")

// Globals
HWND g_hWnd = nullptr;
bool g_visible = false;
bool g_typing = false; // true = editable, false = click-through

ID2D1Factory* g_pFactory = nullptr;
ID2D1HwndRenderTarget* g_pRenderTarget = nullptr;
ID2D1SolidColorBrush* g_pBrush = nullptr;
IDWriteFactory* g_pDWriteFactory = nullptr;
IDWriteTextFormat* g_pTextFormat = nullptr;

std::wstring g_text;
size_t g_cursorPos = 0;
bool g_cursorVisible = true;

DWORD g_lastInputTime = 0;      // last time user typed a key
const DWORD g_cursorBlinkDelay = 500; // 500 ms delay before blinking

// Forward declarations
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void CreateGraphicsResources();
void DiscardGraphicsResources();
void OnPaint();
void ToggleClickThrough(bool enable);


// Entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
    const char CLASS_NAME[] = "ChatOverlay";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    RECT screen;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &screen, 0);

    int width = 600;
    int height = 80;
    int margin = 50;

    int x = (screen.right - screen.left - width) / 2;
    int y = screen.bottom - height - margin;

    g_hWnd = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW,
        CLASS_NAME,
        "Chat Overlay",
        WS_POPUP,
        x, y, width, height,
        nullptr, nullptr, hInstance, nullptr
    );

    // Fully transparent background (0 alpha)
    SetLayeredWindowAttributes(g_hWnd, RGB(0,0,0), 0, LWA_COLORKEY);

    // Register global hotkeys
    RegisterHotKey(nullptr, 1, MOD_ALT, 0x58); // ALT+X toggle
    RegisterHotKey(nullptr, 2, MOD_ALT, 0x51); // ALT+Q quit
    RegisterHotKey(nullptr, 3, 0, VK_ESCAPE);  // ESC hide

    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &g_pFactory);

    DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
    __uuidof(IDWriteFactory),
    reinterpret_cast<IUnknown**>(&g_pDWriteFactory));

    g_pDWriteFactory->CreateTextFormat(
        L"Segoe UI", nullptr,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        18.0f, L"en-us",
        &g_pTextFormat);


    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (msg.message == WM_HOTKEY)
        {
            if (msg.wParam == 1) {
                g_visible = !g_visible;
                ShowWindow(g_hWnd, g_visible ? SW_SHOWNOACTIVATE : SW_HIDE);

                if (g_visible) {
                    ToggleClickThrough(false); // disable click-through while typing
                    ShowWindow(g_hWnd, SW_SHOW);
                    SetForegroundWindow(g_hWnd);  // grab focus
                } else {
                    ShowWindow(g_hWnd, SW_HIDE);
                }
            }
            else if (msg.wParam == 2) {
                PostQuitMessage(0);
            }
            else if (msg.wParam == 3) {
                if (g_visible) {
                    g_visible = false;
                    ShowWindow(g_hWnd, SW_HIDE);
                }
            }
        }

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    DiscardGraphicsResources();
    if (g_pFactory) g_pFactory->Release();
    return 0;
}

void ToggleClickThrough(bool enable)
{
    LONG exStyle = GetWindowLong(g_hWnd, GWL_EXSTYLE);
    if (enable)
        SetWindowLong(g_hWnd, GWL_EXSTYLE, exStyle | WS_EX_TRANSPARENT);
    else
        SetWindowLong(g_hWnd, GWL_EXSTYLE, exStyle & ~WS_EX_TRANSPARENT);
}

void CreateGraphicsResources()
{
    if (g_pRenderTarget == nullptr)
    {
        RECT rc;
        GetClientRect(g_hWnd, &rc);

        D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);

        g_pFactory->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(),
            D2D1::HwndRenderTargetProperties(g_hWnd, size),
            &g_pRenderTarget
        );

        g_pRenderTarget->CreateSolidColorBrush(
            D2D1::ColorF(D2D1::ColorF::DodgerBlue), &g_pBrush);
    }
}

void DiscardGraphicsResources()
{
    if (g_pBrush) { g_pBrush->Release(); g_pBrush = nullptr; }
    if (g_pRenderTarget) { g_pRenderTarget->Release(); g_pRenderTarget = nullptr; }
}

void OnPaint()
{
    CreateGraphicsResources();
    g_pRenderTarget->BeginDraw();

    // Clear the whole render target (fully transparent)
    g_pRenderTarget->Clear(D2D1::ColorF(0, 0));

    RECT rc;
    GetClientRect(g_hWnd, &rc);

    float radius = 15.0f;
    D2D1_ROUNDED_RECT rr = {
        D2D1::RectF(0.5f, 0.5f, (FLOAT)rc.right - 0.5f, (FLOAT)rc.bottom - 0.5f),
        radius, radius
    };

    // Draw outline
    g_pBrush->SetColor(D2D1::ColorF(D2D1::ColorF::DodgerBlue, 1.0f));
    g_pRenderTarget->DrawRoundedRectangle(&rr, g_pBrush, 2.0f);

    // Text layout
    D2D1_RECT_F layout = D2D1::RectF(15, 30, (FLOAT)rc.right - 15, (FLOAT)rc.bottom - 10);
    IDWriteTextLayout* pLayout = nullptr;
    g_pDWriteFactory->CreateTextLayout(
        g_text.c_str(), (UINT32)g_text.size(),
        g_pTextFormat,
        layout.right - layout.left,
        layout.bottom - layout.top,
        &pLayout);

    // --- Fake shadow by drawing text multiple times offset ---
    g_pBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Black, 0.5f));
    const float shadowOffsets[] = { -1.5f, 0.0f, 1.5f }; // simple 3x3 offsets
    for (float dx : shadowOffsets) {
        for (float dy : shadowOffsets) {
            g_pRenderTarget->DrawTextLayout(
                D2D1::Point2F(layout.left + dx + 2, layout.top + dy + 2), // slightly offset shadow
                pLayout,
                g_pBrush
            );
        }
    }

    // Draw main text on top
    g_pBrush->SetColor(D2D1::ColorF(D2D1::ColorF::White));
    g_pRenderTarget->DrawTextLayout(D2D1::Point2F(layout.left, layout.top), pLayout, g_pBrush);

    if (pLayout) pLayout->Release();

    // Draw cursor
    if (g_cursorVisible) {
        FLOAT x = layout.left;

        IDWriteTextLayout* pCursorLayout = nullptr;
        g_pDWriteFactory->CreateTextLayout(
            g_text.c_str(), (UINT32)g_cursorPos,
            g_pTextFormat,
            layout.right - layout.left,
            layout.bottom - layout.top,
            &pCursorLayout);

        DWRITE_TEXT_METRICS metrics;
        pCursorLayout->GetMetrics(&metrics);
        x += metrics.widthIncludingTrailingWhitespace;

        FLOAT cursorTop = layout.top;
        FLOAT cursorBottom = layout.top + metrics.height;

        g_pRenderTarget->DrawLine(
            D2D1::Point2F(x, cursorTop),
            D2D1::Point2F(x, cursorBottom),
            g_pBrush, 1.5f);

        pCursorLayout->Release();
    }

    g_pRenderTarget->EndDraw();
}



LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_PAINT:
            OnPaint();
            ValidateRect(hWnd, nullptr);
            return 0;

        case WM_COMMAND:
            if (HIWORD(wParam) == EN_UPDATE) {
                // typing
            }
            break;
        case WM_CREATE:
            SetTimer(hWnd, 1, 500, nullptr);
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        case WM_TIMER:
        {
            DWORD now = GetTickCount();
            if (now - g_lastInputTime >= g_cursorBlinkDelay) {
                g_cursorVisible = !g_cursorVisible; // toggle only after delay
                InvalidateRect(hWnd, nullptr, TRUE);
            }
            return 0;
        }


        case WM_CHAR:
            g_lastInputTime = GetTickCount(); // reset blink timer

            if (wParam == VK_BACK) {
                if (!g_text.empty() && g_cursorPos > 0) {
                    g_text.erase(g_cursorPos - 1, 1);
                    g_cursorPos--;
                }
            } else if (wParam >= 32) { // printable
                g_text.insert(g_cursorPos, 1, (wchar_t)wParam);
                g_cursorPos++;
            }
            g_cursorVisible = true; // show cursor immediately
            InvalidateRect(hWnd, nullptr, TRUE);
            return 0;

        case WM_KEYDOWN:
            g_lastInputTime = GetTickCount(); // reset blink timer
            g_cursorVisible = true;           // show cursor immediately

            if (wParam == VK_LEFT && g_cursorPos > 0) g_cursorPos--;
            if (wParam == VK_RIGHT && g_cursorPos < g_text.size()) g_cursorPos++;
            if (wParam == VK_RETURN) {
                g_text.clear();
                g_cursorPos = 0;
            }
            InvalidateRect(hWnd, nullptr, TRUE);
            return 0;


    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}



