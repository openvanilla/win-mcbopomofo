#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "Key.h"
#include "Ipc.h"

namespace McBopomofo {

// Utility to convert IPC Key Payload to McBopomofo::Key
Key MapIPCKey(const IPC::KeyEventPayload& payload);

} // namespace McBopomofo
