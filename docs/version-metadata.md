# Version Metadata

This document explains how Win-McBopomofo generates Windows binary version
metadata for its executables and DLLs.

## Source of Truth

The canonical project version is declared in the top-level
`CMakeLists.txt`:

```cmake
project(WinMcBopomofo VERSION 1.0.0 LANGUAGES CXX C)
```

This `project(... VERSION ...)` field is the only version string that should be
edited when the product version changes.

## Generation Flow

During CMake configure, the build runs:

- `cmake/GenerateVersionRc.cmake`

That script:

1. Reads the repository root `CMakeLists.txt`
2. Extracts the `VERSION` value from the `project(...)` declaration
3. Converts the version into Windows resource macros
4. Writes a generated include file to:
   `${CMAKE_BINARY_DIR}/generated/WinMcBopomofoVersion.rcinc`

The generated file currently defines:

```c
#define WINMCBOPOMOFO_VERSION_NUM 1,0,0,0
#define WINMCBOPOMOFO_VERSION_STR "1.0.0.0"
```

## Version Mapping Rule

Windows version resources use a four-part numeric version:

- `major`
- `minor`
- `patch`
- `tweak`

The project version in `CMakeLists.txt` is currently parsed as:

- `major.minor.patch`
- or `major.minor.patch.tweak`

The conversion rules are:

1. If the source version has three components, the generated Windows version
   becomes `major.minor.patch.0`.
2. If the source version has four components, the generated Windows version
   keeps all four values.

Examples:

- `1.0.0` -> `1.0.0.0`
- `2.4.7` -> `2.4.7.0`
- `3.1.5.12` -> `3.1.5.12`

## Where the Generated Version Is Used

The generated resource include is consumed by:

- `src/Client/McBopomofoTIP.rc`
- `src/Server/McBopomofoServer.rc`
- `src/ConfigApp/McBopomofoConfig.rc`

These resource files use the generated macros for:

- `FILEVERSION`
- `PRODUCTVERSION`
- `FileVersion`
- `ProductVersion`

Other metadata fields such as `CompanyName`, `ProductName`,
`FileDescription`, and `OriginalFilename` remain static in each `.rc` file,
because they differ by binary or are not version-derived.

## Why This Exists

This setup avoids duplicating the version number across multiple Windows
resource files. It keeps the build aligned with the CMake project version and
reduces the risk that one binary reports a different version from another.

## Maintenance Rule

When changing the product version:

1. Update the `VERSION` field in the root `CMakeLists.txt`
2. Re-run CMake configure
3. Rebuild the targets

Do not manually edit the generated file under `${CMAKE_BINARY_DIR}`. It is a
build artifact and will be regenerated on the next configure step.
