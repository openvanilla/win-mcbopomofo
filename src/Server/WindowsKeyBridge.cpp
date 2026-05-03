#include "WindowsKeyBridge.h"

namespace McBopomofo {

Key MapIPCKey(const IPC::KeyEventPayload& payload) {
    char ascii = (char)payload.ascii;
    Key::KeyName name = Key::KeyName::UNKNOWN;
    bool isFromNumberPad = false;

    if (payload.ctrl && !payload.shift) {
        switch (payload.vk) {
        case VK_OEM_COMMA: ascii = ','; break;
        case VK_OEM_PERIOD: ascii = '.'; break;
        case '1': ascii = '!'; break;
        case VK_OEM_2: ascii = '/'; break;
        case VK_OEM_1: ascii = ';'; break;
        case VK_OEM_7: ascii = '\''; break;
        case VK_OEM_5: ascii = '\\'; break;
        default: break;
        }
    }
    
    switch (payload.vk) {
    case VK_BACK: ascii = Key::BACKSPACE; break;
    case VK_RETURN: ascii = Key::RETURN; break;
    case VK_ESCAPE: ascii = Key::ESC; break;
    case VK_SPACE: ascii = Key::SPACE; break;
    case VK_TAB: ascii = Key::TAB; break;
    case VK_DELETE: ascii = Key::DELETE; isFromNumberPad = true; break;
    case VK_LEFT: name = Key::KeyName::LEFT; break;
    case VK_RIGHT: name = Key::KeyName::RIGHT; break;
    case VK_UP: name = Key::KeyName::UP; break;
    case VK_DOWN: name = Key::KeyName::DOWN; break;
    case VK_HOME: name = Key::KeyName::HOME; break;
    case VK_END: name = Key::KeyName::END; break;
    case VK_PRIOR: name = Key::KeyName::PAGE_UP; break;
    case VK_NEXT: name = Key::KeyName::PAGE_DOWN; break;
    case VK_NUMPAD0: ascii = '0'; isFromNumberPad = true; break;
    case VK_NUMPAD1: ascii = '1'; isFromNumberPad = true; break;
    case VK_NUMPAD2: ascii = '2'; isFromNumberPad = true; break;
    case VK_NUMPAD3: ascii = '3'; isFromNumberPad = true; break;
    case VK_NUMPAD4: ascii = '4'; isFromNumberPad = true; break;
    case VK_NUMPAD5: ascii = '5'; isFromNumberPad = true; break;
    case VK_NUMPAD6: ascii = '6'; isFromNumberPad = true; break;
    case VK_NUMPAD7: ascii = '7'; isFromNumberPad = true; break;
    case VK_NUMPAD8: ascii = '8'; isFromNumberPad = true; break;
    case VK_NUMPAD9: ascii = '9'; isFromNumberPad = true; break;
    case VK_DECIMAL: ascii = '.'; isFromNumberPad = true; break;
    case VK_ADD: ascii = '+'; isFromNumberPad = true; break;
    case VK_SUBTRACT: ascii = '-'; isFromNumberPad = true; break;
    case VK_MULTIPLY: ascii = '*'; isFromNumberPad = true; break;
    case VK_DIVIDE: ascii = '/'; isFromNumberPad = true; break;
    default: break;
    }

    return Key(ascii, name, payload.shift, payload.ctrl, isFromNumberPad);
}

} // namespace McBopomofo
