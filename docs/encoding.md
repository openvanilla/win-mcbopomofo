# String Encoding Guidelines

This document outlines the string encoding standards for Win-McBopomofo and the rules for interacting with the Windows platform APIs.

## 1. Internal Encoding (UTF-8)

Win-McBopomofo internally uses **UTF-8** for all core logic, data models, and cross-platform compatible components.

- **Engine and Logic**: All classes in the engine (e.g., `gramambular2`) and the primary server controllers (`InputController`, `KeyHandler`, `LanguageModelLoader`) must use `std::string` containing UTF-8 encoded text.
- **State Payloads**: IPC payloads and state transitions handle strings exclusively in UTF-8.
- **Data Files**: All runtime data files (language models, phrase lists, configuration files) are stored and read as UTF-8.

## 2. Platform Encoding (UTF-16)

Windows APIs (Win32, TSF, COM) natively use **UTF-16** (`wchar_t`, `std::wstring`). Use of UTF-16 is restricted to the "Edge" of the application:

- **TSF Interface**: Interaction with the Windows Text Services Framework.
- **UI Rendering**: Drawing text via DirectWrite or standard Win32 GDI/User32 calls.
- **File System**: Windows file paths handled via `std::filesystem::path` (which uses UTF-16 internally on Windows).
- **Resources**: Strings loaded from Windows Resource Tables (.rc) via `LoadStringW`.

## 3. Encapsulation of Conversions

Conversion between UTF-8 and UTF-16 **must not be scattered** throughout the business logic. All conversions must be encapsulated within dedicated helper utilities.

### Standard Helpers

Use the functions defined in `src/Common/UTFHelper.h`:

- `std::wstring Utf8ToUtf16(const std::string& utf8)`
- `std::string Utf16ToUtf8(const std::wstring& utf16)`

### Implementation Rules

1. **Surgical Conversion**: Convert to UTF-16 as late as possible before calling a Windows API, and convert to UTF-8 as soon as possible after receiving data from Windows.
2. **No Literal Mixing**: Avoid mixing `L"wide string"` literals in core logic. Use UTF-8 string literals and convert them only if they must interface with a platform API.
3. **i18n Integration**: When implementing `LocalizedStrings`, the implementation class (e.g., `WinLocalizedStrings`) is responsible for fetching the UTF-16 resource and performing the conversion to UTF-8 before returning the value to the engine.

## 4. Summary Table

| Layer | Type | Encoding |
| :--- | :--- | :--- |
| **Core Engine** | `std::string` | UTF-8 |
| **Input Controller** | `std::string` | UTF-8 |
| **Key Handler** | `std::string` | UTF-8 |
| **Windows API / TSF** | `std::wstring` | UTF-16 |
| **Resource Files (.rc)** | `STRINGTABLE` | UTF-16 (Encoded as UTF-8 with `#pragma code_page(65001)`) |
