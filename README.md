# Win-McBopomofo

Windows port of McBopomofo built on TSF.

## Development Requirements

To build this project, you need to install the following tools:

- **Visual Studio 2026** (or newer) with the "Desktop development with C++" workload. Ensure you select the following individual components:
    - MSVC v145 - VS 2026 C++ x64/x86 build tools
    - MSVC v145 - VS 2026 C++ ARM64 build tools
    - Windows SDK (latest version recommended)
- **CMake** (included in Visual Studio or installed standalone)
- **WiX Toolset** (v7.0 or newer) - Required for building the `.msi` installer. Ensure the WiX binaries are added to your system `PATH`.

### Quick Installation via Winget

You can quickly install the base tools using Windows Package Manager (`winget`):

```powershell
# Install Visual Studio 2026 Community
winget install Microsoft.VisualStudio.2026.Community

# Install CMake
winget install Kitware.CMake

# Install WiX Toolset
winget install wixtoolset.wix

# Install Windows Terminal (Built-in on Win11, recommended for Win10)
winget install Microsoft.WindowsTerminal

# Accept WiX v7 EULA (required for build)
wix eula accept wix7
```

*Note: After installing Visual Studio via `winget`, you must open the Visual Studio Installer to manually select the "Desktop development with C++" workload and the specific ARM64 build tools.*

## Repository Structure

- `src/`: The core source code of the project.
    - `src/Client/`: The TSF TIP (Text Input Processor) DLL. This is the component loaded by host applications (like Notepad).
    - `src/Server/`: The background engine process. Handles key processing and candidate generation.
    - `src/ConfigApp/`: The standalone configuration utility.
    - `src/Common/`: Shared logic used by multiple components (IPC, path utilities, etc.).
- `data/`: Runtime data files, including the language model (`data.txt`), dictionary service definitions, and bopomofo variants.
- `installer/`: Source files for the MSI installer, including WiX definitions and localization strings (`.wxl`).
- `docs/`: Technical documentation and guidelines (translated to English).
- `scripts/`: Internal PowerShell and VBScript utilities for installation, uninstallation, and process management.
- `tests/`: Unit tests and regression tests.
- `third_party/`: External libraries including OpenCC and Marisa.

## Getting Started with Development

### 1. Setup Environment

Ensure all [Requirements](#development-requirements) are met. Run the `winget` commands to install the base tools and accept the WiX EULA.

### 2. Build and Install Locally

For day-to-day development, use the `install.ps1` script to build all architectures and install them to a local `dist/` folder. **Note: This script requires Administrator privileges.**

Note: In an elevated PowerShell terminal:

```powershell
.\install.ps1
```

This script will:

- Stop any running `McBopomofoServer` or `McBopomofoConfig` instances.
- Compile the code for x64, x86, and ARM64.
- Copy all binaries and data to the `dist/` directory.
- Register the TSF DLLs and start the server process.

### 3. Debugging

- **Tracing Logs**: The server runs in the background. To see what's happening in real-time, use the log tracer:

  ```powershell
  Get-Content -Path $env:TEMP\mcbopomofo_server.log -Wait -Tail 20
  ```

- **Settings**: You can trigger the settings app from the language bar menu or by running `dist\McBopomofoConfig.exe`.
- **Iterative Workflow**: After making code changes, simply run `.\install.ps1` again to rebuild and re-register the components.

### 4. Building the Installer

To generate the final MSI installer, run:

```powershell
.\build_msi.ps1
```

The output will be located in `dist/Win-McBopomofo-Installer.msi`.

## Vocabulary and Language Model

The vocabulary and language model data for Win-McBopomofo are derived from the [upstream macOS McBopomofo project](https://github.com/openvanilla/McBopomofo).

**Please report any issues regarding vocabulary, word frequencies, or bopomofo readings to the macOS version's repository.**

## Coding Style

This project follows a consistent coding style enforced by `clang-format`.

- **Configuration**: See `.clang-format` in the root directory.
- **Requirement**: Please ensure your code is formatted correctly before submitting any pull requests.
- **Formatting Command**:

  ```powershell
  # Format all C++ files in the src directory
  Get-ChildItem -Path src -Include *.cpp,*.h -Recurse | ForEach-Object { clang-format -i $_.FullName }
  ```

### Git Commit Convention

This project uses [Conventional Commits](https://www.conventionalcommits.org/).

- **Format**: `<type>(optional scope): <description>`
- **Common Types**:
    - `feat`: A new feature
    - `fix`: A bug fix
    - `docs`: Documentation only changes
    - `style`: Changes that do not affect the meaning of the code (white-space, formatting, etc.)
    - `refactor`: A code change that neither fixes a bug nor adds a feature
    - `chore`: Updating build tasks, package manager configs, etc.

## Common Scripts

- `install.ps1`: developer-oriented build and local staging flow
- `build_msi.ps1`: build the MSI installer
- `scripts/setup.ps1`: install a staged build to a target directory
- `scripts/uninstall.ps1`: unregister and remove an installed build

See [scripts/README.md](C:/Users/user/Works/win-mcbopomofo/scripts/README.md) for script details.

## Basic Build

Example:

```powershell
cmake -S . -B build_verify -A x64
cmake --build build_verify --config Release
```

Important outputs are usually placed under:

- `build_verify/bin/Release/`
- `dist/`

## Notes

- The project uses Windows TSF and includes both TIP client and background server components.
- Some install and registration flows require Administrator privileges.

## Windows Compatibility

- **Supported OS**: Windows 10 and later (x64, x86, and ARM64).
- **Installer policy**: The MSI installer is configured to block installation on versions older than Windows 10.
- **Note**: While the core logic uses standard Win32 and TSF APIs, older versions (like Windows 7 or 8) are not supported and may experience issues with high-DPI scaling, modern UI themes, or TSF host integration.

## Misc

### Registry Verification

You can use the following commands in PowerShell to verify the installation and registration:

- **Get the installed DLL path (COM Registration):**

  ```powershell
  reg query "HKCR\CLSID\{8C9D652A-9B99-4B77-BA9A-3B0F76923B7B}\InProcServer32" /ve
  ```

- **Verify TSF Language Profile (Traditional Chinese):**

  ```powershell
  reg query "HKLM\SOFTWARE\Microsoft\CTF\TIP\{8C9D652A-9B99-4B77-BA9A-3B0F76923B7B}\LanguageProfile\0x0404\{A3668853-2ED4-4D4B-A951-DE1C8B4C0A29}"
  ```

- **Check Server Autorun Entry:**

  ```powershell
  reg query "HKCU\Software\Microsoft\Windows\CurrentVersion\Run" /v "Win-McBopomofo-Server"
  ```

- **Verify TIP Categories (Immersive Support, etc.):**

  ```powershell
  reg query "HKLM\SOFTWARE\Microsoft\CTF\TIP\{8C9D652A-9B99-4B77-BA9A-3B0F76923B7B}\Category\Category\{534C48C1-063E-406F-8F50-F77617E46C9C}\{8C9D652A-9B99-4B77-BA9A-3B0F76923B7B}"
  ```
