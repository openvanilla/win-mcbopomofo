#pragma once

#include <windows.h>
#include "Key.h"

namespace McBopomofo {

// Utility to convert Windows Virtual Keys to McBopomofo::Key
Key MapWindowsKey(WPARAM wParam, LPARAM lParam);

} // namespace McBopomofo
