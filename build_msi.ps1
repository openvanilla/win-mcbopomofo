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

function Find-WixExecutable {
    $candidates = @(
        "C:\Program Files\WiX Toolset v5.0\bin\wix.exe",
        "C:\Program Files\WiX Toolset v6.0\bin\wix.exe",
        "C:\Program Files\WiX Toolset v4.0\bin\wix.exe",
        "C:\Program Files\WiX Toolset v7.0\bin\wix.exe"
    )

    $cmd = Get-Command wix -ErrorAction SilentlyContinue
    if ($cmd -and $cmd.Source) {
        $candidates += $cmd.Source
    }

    foreach ($candidate in $candidates | Select-Object -Unique) {
        if (Test-Path $candidate) {
            return $candidate
        }
    }
    return $null
}

function Find-WixExtension([string]$name, [int]$majorVersion) {
    $baseDir = Join-Path ${env:ProgramFiles} "Common Files\WixToolset\extensions\$name"
    if (-not (Test-Path $baseDir)) {
        return $null
    }

    $versionDirs = Get-ChildItem $baseDir -Directory -ErrorAction SilentlyContinue |
        Sort-Object { [version]$_.Name } -Descending

    foreach ($dir in $versionDirs) {
        $extPath = Join-Path $dir.FullName ("wixext{0}\{1}.dll" -f $majorVersion, $name)
        if (Test-Path $extPath) {
            return $extPath
        }
    }
    return $null
}

function Invoke-Wix4Build([string]$WixExe, [string]$MsiPath, [string]$BinDir, [string]$OpenCCDir) {
    $uiRef = "WixToolset.UI.wixext/4.0.5"
    $utilRef = "WixToolset.Util.wixext/4.0.5"

    & $WixExe extension add $uiRef -g | Out-Null
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to add WiX extension $uiRef"
    }

    & $WixExe extension add $utilRef -g | Out-Null
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to add WiX extension $utilRef"
    }

    & $WixExe build `
        -ext $uiRef `
        -ext $utilRef `
        .\installer.wxs `
        -o $MsiPath `
        -b "BinDir=$BinDir" `
        -b "OpenCCDir=$OpenCCDir"
}

$WixExe = Find-WixExecutable
if (-not $WixExe) {
    Write-Host "Error: WiX CLI is not installed." -ForegroundColor Red
    Write-Host "Install WiX Toolset Command-Line Tools first." -ForegroundColor Yellow
    exit 1
}

$WixVersionText = & $WixExe --version
$WixMajorVersion = [int](($WixVersionText -split '\.')[0])
if ($WixMajorVersion -ge 7) {
    Write-Host "Error: WiX $WixVersionText requires OSMF acceptance and is not supported by this script." -ForegroundColor Red
    Write-Host "Install WiX CLI 5.x or 6.x instead." -ForegroundColor Yellow
    exit 1
}

Write-Host "Building MSI using WiX $WixVersionText..." -ForegroundColor Cyan

# Ensure output directory exists
$OutDir = "dist"
if (-not (Test-Path $OutDir)) {
    New-Item -ItemType Directory -Path $OutDir | Out-Null
}

$MsiPath = "$OutDir\$OutputName"

if ($WixMajorVersion -eq 4) {
    try {
        Invoke-Wix4Build -WixExe $WixExe -MsiPath $MsiPath -BinDir $BinDir -OpenCCDir $OpenCCDir
    } catch {
        Write-Host "Error: $($_.Exception.Message)" -ForegroundColor Red
        Write-Host "WiX 4 requires the UI and Util extensions to be restored into the global extension cache." -ForegroundColor Yellow
        exit 1
    }
} else {
    $UiExtension = Find-WixExtension "WixToolset.UI.wixext" $WixMajorVersion
    $UtilExtension = Find-WixExtension "WixToolset.Util.wixext" $WixMajorVersion
    if (-not $UiExtension -or -not $UtilExtension) {
        Write-Host "Error: Matching WiX extensions were not found for WiX $WixVersionText." -ForegroundColor Red
        Write-Host "Install WiX Additional Tools for the same major version." -ForegroundColor Yellow
        exit 1
    }

    # Build the MSI directly using 'wix build'
    # Pass bindpaths for BinDir and OpenCCDir
    & $WixExe build `
        -ext $UiExtension `
        -ext $UtilExtension `
        .\installer.wxs `
        -o $MsiPath `
        -b "BinDir=$BinDir" `
        -b "OpenCCDir=$OpenCCDir"
}

if ($LASTEXITCODE -eq 0) {
    Write-Host "Successfully created MSI at: $MsiPath" -ForegroundColor Green
} else {
    Write-Host "Failed to build MSI. See output for details." -ForegroundColor Red
    exit $LASTEXITCODE
}
