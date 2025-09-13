#pragma once
#include <windows.h>

class HotkeyManager {
public:
    bool Register(int id, UINT mod, UINT vk) {
        return RegisterHotKey(nullptr, id, mod, vk);
    }
    ~HotkeyManager() {
        UnregisterHotKey(nullptr, 1);
        UnregisterHotKey(nullptr, 2);
        UnregisterHotKey(nullptr, 3);
    }
};
