# Scripts

This directory contains helper scripts that are useful during development, installation, and cleanup.

Scripts in the repository root:

- `install.ps1`: build all required binaries, stage files into `dist/`, register the TSF DLLs locally, restart TSF, and start `McBopomofoServer`.
- `build_msi.ps1`: build the MSI installer from the staged binaries.

Scripts in this directory:

- `setup.ps1`: install an already-staged `dist/` build into a target directory such as `%ProgramFiles%\McBopomofo`, register the TSF DLLs, configure auto-start, and start the server.
- `uninstall.ps1`: unregister installed TSF DLLs, remove auto-start, and optionally delete the installation directory.
- `close_ime_apps.ps1`: find user processes that currently load the IME DLL and try to close or restart them to release file locks before rebuilding or reinstalling.
- `enable_tip.ps1`: add the Win-McBopomofo TIP to the current user's Traditional Chinese input methods list.

Notes:

- `setup.ps1` and `uninstall.ps1` are intended for machine-level install/uninstall flows.
- `install.ps1` is the developer-oriented entry point when working from the source tree.
