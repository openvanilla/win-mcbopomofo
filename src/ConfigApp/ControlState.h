#pragma once

#include <windows.h>

namespace McBopomofo::ConfigApp {

bool IsButtonChecked(HWND control, bool userDataBacked);
void SetButtonChecked(HWND control, bool checked, bool userDataBacked);
void ToggleButtonChecked(HWND control, bool userDataBacked);

}  // namespace McBopomofo::ConfigApp
