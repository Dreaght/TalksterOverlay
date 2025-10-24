#pragma once
#include <windows.h>
#include <map>
#include <string>
#include <vector>
#include "HotkeyManager.h"

class HotkeyRebinder {
public:
    HotkeyRebinder(HINSTANCE hInstance, HotkeyManager& manager);
    ~HotkeyRebinder();

    void Show();

    HWND GetHWND() const { return hWnd; }

private:
    struct HotkeyBinding {
        int id;
        std::string name;
        UINT modifiers;
        UINT vk;
    };

    HINSTANCE hInstance;
    HWND hWnd{};
    HotkeyManager& manager;
    std::vector<HotkeyBinding> bindings;
    int selected{-1};
    bool capturing{false};
    std::vector<UINT> capturedKeys;

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void Render(HWND hWnd, HDC hdc);
    void StartCapture(int index);
    void StopCapture();
    void ApplyCapture();
};
