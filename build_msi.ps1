# build_msi.ps1
# Script to build the MSI installer using WiX v4
param(
    [string]$Configuration = "Release",
    [string]$OutputName = "Win-McBopomofo-Installer.msi"
)

$ErrorActionPreference = "Stop"

# Ensure the build artifacts exist
$BuildRoot = "build"
if (-not (Test-Path $BuildRoot)) {
    if (Test-Path "build_x64") {
        $BuildRoot = "build_x64"
    } else {
        Write-Host "Error: Cannot find build directory (build or build_x64). Please build the project first." -ForegroundColor Red
        exit 1
    }
}

$BinDir = "$BuildRoot\bin\$Configuration"
$OpenCCDir = "$BuildRoot\third_party\OpenCC\data"

if (-not (Test-Path "$BinDir\McBopomofoTIP_v2.dll")) {
    Write-Host "Error: Cannot find built artifacts in $BinDir. Please build the project first." -ForegroundColor Red
    exit 1
}

# Ensure Wix is available and initialized
try {
    wix --version | Out-Null
    
    # Ensure extensions are added
    wix extension add WixToolset.UI.wixext/4.0.5 -g | Out-Null
    wix extension add WixToolset.Util.wixext/4.0.5 -g | Out-Null
} catch {
    Write-Host "Error: WiX v4 tool is not installed or not in PATH." -ForegroundColor Red
    Write-Host "Please run 'dotnet tool install --global wix --version 4.0.5' first." -ForegroundColor Yellow
    exit 1
}

Write-Host "Building MSI using WiX v4..." -ForegroundColor Cyan

# Ensure output directory exists
$OutDir = "dist"
if (-not (Test-Path $OutDir)) {
    New-Item -ItemType Directory -Path $OutDir | Out-Null
}

$MsiPath = "$OutDir\$OutputName"

# Build the MSI directly using 'wix build'
# Pass bindpaths for BinDir and OpenCCDir
wix build -ext WixToolset.UI.wixext -ext WixToolset.Util.wixext .\installer.wxs -o $MsiPath -b "BinDir=$BinDir" -b "OpenCCDir=$OpenCCDir"

if ($LASTEXITCODE -eq 0) {
    Write-Host "Successfully created MSI at: $MsiPath" -ForegroundColor Green
} else {
    Write-Host "Failed to build MSI. See output for details." -ForegroundColor Red
    exit $LASTEXITCODE
}
