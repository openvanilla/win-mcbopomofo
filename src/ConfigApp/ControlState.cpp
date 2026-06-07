#include "ControlState.h"

namespace McBopomofo::ConfigApp {

bool IsButtonChecked(HWND control, bool userDataBacked) {
  if (userDataBacked) {
    return GetWindowLongPtrW(control, GWLP_USERDATA) != 0;
  }
  return SendMessageW(control, BM_GETCHECK, 0, 0) == BST_CHECKED;
}

void SetButtonChecked(HWND control, bool checked, bool userDataBacked) {
  if (userDataBacked) {
    SetWindowLongPtrW(control, GWLP_USERDATA, checked ? 1 : 0);
  }
  SendMessageW(control, BM_SETCHECK, checked ? BST_CHECKED : BST_UNCHECKED, 0);
  InvalidateRect(control, nullptr, TRUE);
}

void ToggleButtonChecked(HWND control, bool userDataBacked) {
  SetButtonChecked(control, !IsButtonChecked(control, userDataBacked),
                   userDataBacked);
}

}  // namespace McBopomofo::ConfigApp
