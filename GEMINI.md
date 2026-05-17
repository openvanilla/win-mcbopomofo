# Win-McBopomofo Development Guidelines

## Core Principles

* **Core Engine Protection**: Modifying `src/Engine` and its related core algorithm code (e.g., `gramambular2`) is strictly prohibited.
* **Adaptation and Bridging**: All adjustments for the Windows platform (TSF, Win32 API) should be implemented in the Adapter/Bridge Layer and must not intrude into the core logic.
* **Language Standard**: Use C++20 standard.
* **Communication Guidelines**: All conversations and documentation must be written in **English**.
* **Test-Driven Development (TDD)**: Always follow Kent Beck's TDD flow.
    * Write tests before adding any feature.
    * Implement the feature.
    *   Fix until the test passes.
    *   Clean up warnings.
    *   Make sure the project compiles.
    *   **Commit Convention**: Use **Conventional Commits** for all changes.
    *   Format: `<type>(<scope>): <description>`
    *   Types: `feat`, `fix`, `docs`, `style`, `refactor`, `perf`, `test`, `build`, `ci`, `chore`, `revert`.
    *   Example: `feat(server): add support for character info service`

## Development Environment & Toolchain

* **IDE and Compiler**: This project is developed using **Visual Studio 2026** (MSVC v145).
* **Build System**: **CMake** is used for project build management.
* **Packaging Tool**: **WiX Toolset v7.0** is used to build the MSI installer. Note: You must first accept the license by running `wix eula accept wix7`.

## Installer Conventions

* **WiX Extensions**: The MSI build depends on WiX v7 extensions `WixToolset.UI.wixext/7.0.0` and `WixToolset.Util.wixext/7.0.0`. `build_msi.ps1` is responsible for installing and passing these extension references to `wix build`.
* **Installer Verification**: After changing `installer/*.wxs`, localization `.wxl` files, or `build_msi.ps1`, verify with `.\build_msi.ps1 -SkipBuild -OutputName Win-McBopomofo-Installer-verify.msi` when the required binaries already exist.
* **License RTF Generation**: `build_msi.ps1` converts `LICENSE.txt` to `build_msi_generated\LICENSE.rtf`. Single line breaks in the source text are treated as hard-wrapped text and must be joined with spaces inside the same paragraph. Blank lines split paragraphs, and generated RTF should use `\par\par` between paragraphs so the WiX license dialog shows one empty line between paragraphs.
* **Upgrade UI**: Major upgrades are detected via `WIX_UPGRADE_DETECTED`. During an upgrade, the installer must not show `InstallDirDlg` or allow editing `INSTALLFOLDER`; the UI should jump from `LicenseAgreementDlg` directly to `VerifyReadyDlg`, and Back should return to the license dialog.
* **Reboot Handling**: Do not suppress reboot prompts for application shutdown during install or upgrade. `util:CloseApplication` entries should keep `RebootPrompt="yes"` so users are prompted if a running process cannot be closed and a reboot is required. Windows Installer may also prompt at the end when locked files need replacement.

## Architecture Conventions

* **Multi-Architecture Support**: This project supports `x86`, `x64`, and `ARM64`.
* **Executable Suffixes**: Aside from the main binaries (`McBopomofoTIP_v2.dll`, `McBopomofoServer.exe`), auxiliary tools like the configuration app (`McBopomofoConfig.exe`) must be architecture-aware when packaged and executed.
* **Discovery Logic**: When the program needs to launch an external executable, it should first attempt to find the generic name (e.g., `McBopomofoConfig.exe`). If not found, it should fall back to the architecture-specific name with a suffix (e.g., `McBopomofoConfig_arm64.exe`) based on the current compilation macros (`_M_IX86`, `_M_X64`/`_M_AMD64`, `_M_ARM64`).

## OS Support

* **Target OS**: Only **Windows 10 and newer versions** are supported.
* **Compatibility**: Installation on older operating systems like Windows 7 / 8 is not supported and is actively blocked. When dealing with UI (High DPI) or TSF integration issues, modern APIs for Windows 10/11 should be used as the baseline.
