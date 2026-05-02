#include "WindowsKeyBridge.h"

namespace McBopomofo {

Key MapWindowsKey(WPARAM wParam, LPARAM lParam) {
    char ascii = 0;
    Key::KeyName name = Key::KeyName::UNKNOWN;
    bool shiftPressed = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
    bool ctrlPressed = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    bool isFromNumberPad = false;

    switch (wParam) {
    case VK_BACK: ascii = Key::BACKSPACE; break;
    case VK_RETURN: ascii = Key::RETURN; break;
    case VK_ESCAPE: ascii = Key::ESC; break;
    case VK_SPACE: ascii = Key::SPACE; break;
    case VK_TAB: ascii = Key::TAB; break;
    case VK_DELETE: ascii = 127; break; // Use literal since DELETE is undef
    case VK_LEFT: name = Key::KeyName::LEFT; break;
    case VK_RIGHT: name = Key::KeyName::RIGHT; break;
    case VK_UP: name = Key::KeyName::UP; break;
    case VK_DOWN: name = Key::KeyName::DOWN; break;
    case VK_HOME: name = Key::KeyName::HOME; break;
    case VK_END: name = Key::KeyName::END; break;
    // PAGE_UP and PAGE_DOWN are not in KeyName enum
    // case VK_PRIOR: name = Key::KeyName::PAGE_UP; break;
    // case VK_NEXT: name = Key::KeyName::PAGE_DOWN; break;
    default:
        // Handle alphanumeric and punctuation
        BYTE keyboardState[256];
        GetKeyboardState(keyboardState);
        WCHAR chars[2];
        int res = ToUnicode((UINT)wParam, (lParam >> 16) & 0xFF, keyboardState, chars, 2, 0);
        if (res == 1) {
            // Very simple ASCII mapping for English layout
            if (chars[0] >= 32 && chars[0] <= 126) {
                ascii = (char)chars[0];
            }
        }
        break;
    }

    return Key(ascii, name, shiftPressed, ctrlPressed, isFromNumberPad);
}

} // namespace McBopomofo
