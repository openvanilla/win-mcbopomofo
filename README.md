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

# Accept WiX v7 EULA (required for build)
wix eula accept wix7
```

*Note: After installing Visual Studio via `winget`, you must open the Visual Studio Installer to manually select the "Desktop development with C++" workload and the specific ARM64 build tools.*

## What This Repo Contains

- `src/`: client, server, config app, and shared code
- `data/`: language model and runtime data files
- `tests/`: local test and experiment code
- `scripts/`: helper scripts for setup, uninstall, and development utilities
- `third_party/`: vendored dependencies such as OpenCC

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

- Command to get installed path:

```powershell
reg query "HKCR\CLSID\{8C9D652A-9B99-4B77-BA9A-3B0F76923B7B}\InProcServer32" /ve
```
