#include "HotkeySetup.h"

namespace App {
    void SetupHotkeys(HotkeyManager& hotkeys) {
        hotkeys.Register(1, MOD_ALT, 'X'); // toggle chat
        hotkeys.Register(2, MOD_ALT, 'Q'); // quit
        hotkeys.Register(3, 0, VK_ESCAPE); // hide chat
    }
}
