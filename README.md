# Win-McBopomofo

Windows port of McBopomofo built on TSF.

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

- Officially supported: `Windows 10` and later.
- Installer policy: the MSI installer blocks installation on versions older than Windows 10.
- Practical compatibility note:
  The core code mostly relies on Win32, TSF, Direct2D, and DirectWrite APIs that can exist on older systems such as Windows 7 SP1.
  However, older Windows versions are not part of the supported matrix, are not covered by installer support, and may have runtime issues in UI behavior, theming, DPI handling, or TSF host compatibility.

## Misc

- Command to get installed path:

```powershell
reg query "HKCR\CLSID\{8C9D652A-9B99-4B77-BA9A-3B0F76923B7B}\InProcServer32" /ve
```