#include "WindowsKeyBridge.h"

namespace McBopomofo {

Key MapIPCKey(const IPC::KeyEventPayload& payload) {
    char ascii = (char)payload.ascii;
    Key::KeyName name = Key::KeyName::UNKNOWN;
    
    switch (payload.vk) {
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
    case VK_PRIOR: name = Key::KeyName::PAGE_UP; break;
    case VK_NEXT: name = Key::KeyName::PAGE_DOWN; break;
    default:
        break;
    }

    return Key(ascii, name, payload.shift, payload.ctrl, false);
}

} // namespace McBopomofo
