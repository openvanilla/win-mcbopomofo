// Copyright (c) 2026 and onwards The McBopomofo Authors.
//
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use,
// copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following
// conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.

#include "WindowsKeyBridge.h"

namespace McBopomofo {

char mapCtrlPunctuationAscii_(unsigned int vk) {
  switch (vk) {
    case VK_OEM_COMMA:
      return ',';
    case VK_OEM_PERIOD:
      return '.';
    case '1':
      return '!';
    case VK_OEM_2:
      return '/';
    case VK_OEM_1:
      return ';';
    case VK_OEM_7:
      return '\'';
    case VK_OEM_5:
      return '\\';
    default:
      return '\0';
  }
}

Key mapIpcKey(const IPC::KeyEventPayload& payload) {
  char ascii = (char)payload.ascii;
  Key::KeyName name = Key::KeyName::UNKNOWN;
  bool isFromNumberPad = false;

  if (payload.ctrl) {
    char ctrlPunctuationAscii = mapCtrlPunctuationAscii_(payload.vk);
    if (ctrlPunctuationAscii != '\0') {
      ascii = ctrlPunctuationAscii;
    }
  }

  switch (payload.vk) {
    case VK_BACK:
      ascii = Key::BACKSPACE;
      break;
    case VK_RETURN:
      ascii = Key::RETURN;
      break;
    case VK_ESCAPE:
      ascii = Key::ESC;
      break;
    case VK_SPACE:
      ascii = Key::SPACE;
      break;
    case VK_TAB:
      ascii = Key::TAB;
      break;
    case VK_DELETE:
      ascii = Key::DELETE;
      isFromNumberPad = true;
      break;
    case VK_LEFT:
      name = Key::KeyName::LEFT;
      break;
    case VK_RIGHT:
      name = Key::KeyName::RIGHT;
      break;
    case VK_UP:
      name = Key::KeyName::UP;
      break;
    case VK_DOWN:
      name = Key::KeyName::DOWN;
      break;
    case VK_HOME:
      name = Key::KeyName::HOME;
      break;
    case VK_END:
      name = Key::KeyName::END;
      break;
    case VK_PRIOR:
      name = Key::KeyName::PAGE_UP;
      break;
    case VK_NEXT:
      name = Key::KeyName::PAGE_DOWN;
      break;
    case VK_NUMPAD0:
      ascii = '0';
      isFromNumberPad = true;
      break;
    case VK_NUMPAD1:
      ascii = '1';
      isFromNumberPad = true;
      break;
    case VK_NUMPAD2:
      ascii = '2';
      isFromNumberPad = true;
      break;
    case VK_NUMPAD3:
      ascii = '3';
      isFromNumberPad = true;
      break;
    case VK_NUMPAD4:
      ascii = '4';
      isFromNumberPad = true;
      break;
    case VK_NUMPAD5:
      ascii = '5';
      isFromNumberPad = true;
      break;
    case VK_NUMPAD6:
      ascii = '6';
      isFromNumberPad = true;
      break;
    case VK_NUMPAD7:
      ascii = '7';
      isFromNumberPad = true;
      break;
    case VK_NUMPAD8:
      ascii = '8';
      isFromNumberPad = true;
      break;
    case VK_NUMPAD9:
      ascii = '9';
      isFromNumberPad = true;
      break;
    case VK_DECIMAL:
      ascii = '.';
      isFromNumberPad = true;
      break;
    case VK_ADD:
      ascii = '+';
      isFromNumberPad = true;
      break;
    case VK_SUBTRACT:
      ascii = '-';
      isFromNumberPad = true;
      break;
    case VK_MULTIPLY:
      ascii = '*';
      isFromNumberPad = true;
      break;
    case VK_DIVIDE:
      ascii = '/';
      isFromNumberPad = true;
      break;
    default:
      break;
  }

  return Key(ascii, name, payload.shift, payload.ctrl, isFromNumberPad);
}

}  // namespace McBopomofo
