#pragma once
#include <windows.h>

class HotkeyManager {
public:
    bool Register(int id, UINT mod, UINT vk) {
        return RegisterHotKey(nullptr, id, mod, vk);
    }
    ~HotkeyManager() {
        UnregisterAll();
    }

    static void UnregisterAll() {
        UnregisterHotKey(nullptr, 1);
        UnregisterHotKey(nullptr, 2);
        UnregisterHotKey(nullptr, 3);
    }
};
