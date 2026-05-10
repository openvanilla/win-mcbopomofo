# Win-McBopomofo Logging Guidelines

## 1. Overview

Win-McBopomofo provides a lightweight, thread-safe, stream-based logging mechanism. The logging system is primarily designed for the Server process (Input Method Core Engine) to facilitate debugging, track IPC messages, and record runtime errors.

## 2. Log File Location

The log file is written to the user's temporary directory.

- **Path**: `%TEMP%\mcbopomofo_server.log`
- **Resolution**: Uses the Windows API `GetTempPathW()` combined with `mcbopomofo_server.log`.

*Note: Since the server runs in the background and is often launched via COM/TSF, writing to the temporary directory ensures that the process always has the necessary write permissions.*

## 3. Log Format

Each log entry follows this structure:

```text
[<ProcessId>] [<LEVEL>] <Message>
```

**Example:**

```text
[14820] [INFO] IPC Recv: VK=65, ASCII=97
[14820] [ERROR] Failed to load language model from data/data.txt
```

## 4. Usage

To write logs, include the header `src/Server/Log.h` and use one of the provided macros. The macros return a context object that overrides the `<<` operator, allowing standard C++ stream manipulation.

### Available Macros

The macros retain the `FCITX_MCBOPOMOFO_` prefix, reflecting the project's historical roots.

- `FCITX_MCBOPOMOFO_ERROR()`: For critical failures and exceptions.
- `FCITX_MCBOPOMOFO_WARN()`: For non-critical issues or unexpected states.
- `FCITX_MCBOPOMOFO_INFO()`: For general information (e.g., IPC connections, state changes).
- `FCITX_MCBOPOMOFO_DEBUG()`: For verbose diagnostic information.

### Examples

```cpp
#include "Log.h"

// Basic logging
FCITX_MCBOPOMOFO_INFO() << "Server started successfully.";

// Logging with variables
int vkCode = 65;
FCITX_MCBOPOMOFO_DEBUG() << "Received virtual key code: " << vkCode;

// Logging errors
if (!fileLoaded) {
    FCITX_MCBOPOMOFO_ERROR() << "Failed to load configuration from path: " << configPath;
}
```

## 5. Controlling Logging State

Logging is enabled by default (`true`). However, it can be dynamically enabled or disabled at runtime to reduce I/O overhead or disk space consumption.

The following functions are available in the `McBopomofo` namespace:

- `bool ServerLoggingEnabled()`: Returns the current logging state.
- `void SetServerLoggingEnabled(bool enabled)`: Sets the logging state (uses `std::atomic_bool` for thread safety).

**Example:**

```cpp
// Disable logging in release builds to save I/O
McBopomofo::SetServerLoggingEnabled(false);
```

### Enabling Logging in the Server

In `src/Server/main.cpp`, logging is typically initialized early in the process lifecycle. If you need to forcefully enable or disable logging for debugging purposes across the entire server instance, you should call `McBopomofo::SetServerLoggingEnabled(true);` right at the beginning of the `wWinMain` or `main` entry point.

## 6. Tracing Logs in a Terminal

Because the server runs in the background and does not have a console window attached, the best way to view logs in real-time is to trace the log file using a terminal.

Open PowerShell and use the `Get-Content` cmdlet with the `-Wait` (and `-Tail`) parameters to stream the log file as it updates:

```powershell
Get-Content -Path $env:TEMP\mcbopomofo_server.log -Wait -Tail 20
```

*Tip: Press `Ctrl + C` to stop tracing. This is extremely useful for monitoring IPC events and key handling as you type in the target application.*

## 7. Implementation Details

- **Header**: `src/Server/Log.h`
- **Source**: `src/Server/Log.cpp`
- **Mechanism**: The macro instantiates a temporary `LogMessageContext` object. As data is streamed into it using `operator<<`, it accumulates in an internal `std::ostringstream`. When the full statement completes, the destructor `~LogMessageContext()` is triggered, which opens the file in append mode (`"a"`), writes the formatted string, and closes the file immediately.
