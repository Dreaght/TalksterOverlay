#include "HotkeyRebinder.h"
#include <sstream>

#define IDC_LIST 2001

HotkeyRebinder::HotkeyRebinder(HINSTANCE inst, HotkeyManager& mgr)
    : hInstance(inst), manager(mgr)
{
    bindings = {
        {1, "Toggle Chat", MOD_ALT, 'X'},
        {2, "Quit App",    MOD_ALT, 'Q'},
    };

    WNDCLASSA wc{};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "HotkeyRebinderWnd";
    RegisterClassA(&wc);
}

HotkeyRebinder::~HotkeyRebinder() {
    if (hWnd) DestroyWindow(hWnd);
}

void HotkeyRebinder::Show() {
    hWnd = CreateWindowA("HotkeyRebinderWnd", "Edit Key Bindings",
        WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME ^ WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 360, 240,
        nullptr, nullptr, hInstance, this);
    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);
}

LRESULT CALLBACK HotkeyRebinder::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    auto self = reinterpret_cast<HotkeyRebinder*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    switch (msg) {
    case WM_CREATE:
        SetWindowLongPtr(hWnd, GWLP_USERDATA,
            reinterpret_cast<LONG_PTR>(((CREATESTRUCT*)lParam)->lpCreateParams));
        return 0;

    case WM_KEYDOWN:
        if (self && self->capturing) {
            if (wParam == VK_RETURN) {
                self->ApplyCapture();
            } else if (wParam == VK_ESCAPE) {
                self->StopCapture();
            } else {
                if (self->capturedKeys.size() < 3)
                    self->capturedKeys.push_back((UINT)wParam);
            }
            InvalidateRect(hWnd, nullptr, TRUE);
        }
        return 0;

    case WM_LBUTTONDOWN:
        if (self && !self->capturing) {
            int y = HIWORD(lParam);
            int index = (y - 30) / 25;
            if (index >= 0 && index < (int)self->bindings.size())
                self->StartCapture(index);
        }
        return 0;

    case WM_PAINT:
        if (self) {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            self->Render(hWnd, hdc);
            EndPaint(hWnd, &ps);
        }
        return 0;

    case WM_DESTROY:
        if (self) self->StopCapture();
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

void HotkeyRebinder::Render(HWND, HDC hdc) {
    int y = 30;
    for (size_t i = 0; i < bindings.size(); ++i) {
        const auto& b = bindings[i];
        std::ostringstream label;
        label << b.name << "  [";

        if (capturing && (int)i == selected) {
            label << "Press keys...";
        } else {
            label << ((b.modifiers & MOD_CONTROL) ? "CTRL+" : "")
                  << ((b.modifiers & MOD_ALT) ? "ALT+" : "")
                  << (char)b.vk;
        }
        label << "]";
        TextOutA(hdc, 20, y, label.str().c_str(), (int)label.str().length());
        y += 25;
    }
}

void HotkeyRebinder::StartCapture(int index) {
    selected = index;
    capturing = true;
    capturedKeys.clear();
    InvalidateRect(hWnd, nullptr, TRUE);
}

void HotkeyRebinder::StopCapture() {
    capturing = false;
    selected = -1;
    capturedKeys.clear();
    InvalidateRect(hWnd, nullptr, TRUE);
}

void HotkeyRebinder::ApplyCapture() {
    if (capturedKeys.empty()) {
        StopCapture();
        return;
    }
    auto& b = bindings[selected];

    UINT mod = 0;
    for (UINT key : capturedKeys) {
        if (key == VK_CONTROL) mod |= MOD_CONTROL;
        else if (key == VK_MENU) mod |= MOD_ALT;
        else b.vk = key;
    }

    b.modifiers = mod;

    manager.UnregisterAll();
    for (auto& bk : bindings)
        manager.Register(bk.id, bk.modifiers, bk.vk);

    StopCapture();
}
