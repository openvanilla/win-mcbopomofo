# build_msi.ps1
# Script to build a Win-McBopomofo MSI installer
#
param(
    [string]$Configuration = "Release",
    [string]$OutputName = "Win-McBopomofo-Installer.msi",
    [switch]$SkipBuild = $false
)

$ErrorActionPreference = "Stop"

$X64BuildRoot = "build_x64"
$X86BuildRoot = "build_x86"
$Arm64BuildRoot = "build_arm64"

$X64BinDir = "$X64BuildRoot\bin\$Configuration"
$X86BinDir = "$X86BuildRoot\bin\$Configuration"
$Arm64BinDir = "$Arm64BuildRoot\bin\$Configuration"
$OpenCCDir = "$X64BuildRoot\third_party\OpenCC\data"
$GeneratedDir = "build_msi_generated"
$LicenseTxtPath = "LICENSE.txt"
$LicenseRtfPath = Join-Path $GeneratedDir "LICENSE.rtf"

# Detect current platform
function Get-CurrentPlatform {
    $processorArchitecture = $env:PROCESSOR_ARCHITECTURE
    $processorArchW6432 = $env:PROCESSOR_ARCHITEW6432
    if ($null -eq $processorArchW6432) {
        switch ($processorArchitecture) {
            "AMD64" { return "x64" }
            "ARM64" { return "ARM64" }
            "x86" { return "x86" }
            default { return $processorArchitecture }
        }
    } else {
        switch ($processorArchW6432) {
            "AMD64" { return "x64" }
            "ARM64" { return "ARM64" }
            default { return "x64" }
        }
    }
}

$CurrentPlatform = Get-CurrentPlatform
Write-Host "Detected current platform: $CurrentPlatform" -ForegroundColor Cyan

function Should-SkipOpenCCDict {
    param([string]$TargetArchitecture, [string]$CurrentPlatform)
    return ($TargetArchitecture -ne $CurrentPlatform)
}

function Get-CMakeCacheValue {
    param([string]$CachePath, [string]$VariableName)
    if (-not (Test-Path $CachePath)) { return $null }
    $match = Select-String -Path $CachePath -Pattern "^$([regex]::Escape($VariableName)):.*=(.*)$" -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($null -eq $match) { return $null }
    return $match.Matches[0].Groups[1].Value
}

$requiredArtifacts = @(
    "$X64BinDir\McBopomofoServer.exe",
    "$X64BinDir\McBopomofoConfig.exe",
    "$X64BinDir\McBopomofoTIP_v2.dll",
    "$X86BinDir\McBopomofoServer.exe",
    "$X86BinDir\McBopomofoConfig.exe",
    "$X86BinDir\McBopomofoTIP_v2.dll",
    "$Arm64BinDir\McBopomofoServer.exe",
    "$Arm64BinDir\McBopomofoConfig.exe",
    "$Arm64BinDir\McBopomofoTIP_v2.dll"
)

function Build-Architecture([string]$Architecture, [string]$BuildRoot, [string]$Configuration) {
    Write-Host "Building $Architecture architecture in $BuildRoot..." -ForegroundColor Cyan
    if (-not (Test-Path $BuildRoot)) { New-Item -ItemType Directory -Path $BuildRoot | Out-Null }
    Push-Location $BuildRoot
    try {
        $skipOpenCCDict = Should-SkipOpenCCDict -TargetArchitecture $Architecture -CurrentPlatform $CurrentPlatform
        $skipOpenCCDictFlag = if ($skipOpenCCDict) { "-DSKIP_OPENCC_DICT=ON" } else { "-DSKIP_OPENCC_DICT=OFF" }
        $cachePath = Join-Path $BuildRoot "CMakeCache.txt"
        $cachedSkipOpenCCDict = Get-CMakeCacheValue -CachePath $cachePath -VariableName "SKIP_OPENCC_DICT"
        $needsConfigure = $true
        if ($cachedSkipOpenCCDict -ne $null) {
            if (($skipOpenCCDict -and $cachedSkipOpenCCDict.Trim() -eq "ON") -or (-not $skipOpenCCDict -and $cachedSkipOpenCCDict.Trim() -eq "OFF")) { $needsConfigure = $false }
        }
        if ($needsConfigure) {
            $cmakeArgs = @($skipOpenCCDictFlag, "-DCMAKE_BUILD_TYPE=$Configuration")
            if ($Architecture -eq "ARM64") { cmake -A ARM64 @cmakeArgs .. }
            elseif ($Architecture -eq "x86") { cmake -A Win32 @cmakeArgs .. }
            else { cmake -A x64 @cmakeArgs .. }
            if ($LASTEXITCODE -ne 0) { throw "CMake configure failed" }
        }
        if ($Architecture -eq "x64") {
            cmake --build . --config $Configuration --target third_party/OpenCC/data/Dictionaries
        }
        cmake --build . --config $Configuration --target McBopomofoTIP McBopomofoServer McBopomofoConfig
        if ($LASTEXITCODE -ne 0) { throw "CMake build failed" }
    } finally { Pop-Location }
}

function Check-AndBuildMissingArtifacts {
    $missing = $requiredArtifacts | Where-Object { -not (Test-Path $_) }
    if ($missing.Count -gt 0) {
        if ($SkipBuild) { Write-Host "Error: Artifacts missing and -SkipBuild set." -ForegroundColor Red; exit 1 }
        if ($missing | Where-Object { $_ -like "*$X64BuildRoot*" }) { Build-Architecture "x64" $X64BuildRoot $Configuration }
        if ($missing | Where-Object { $_ -like "*$X86BuildRoot*" }) { Build-Architecture "x86" $X86BuildRoot $Configuration }
        if ($missing | Where-Object { $_ -like "*$Arm64BuildRoot*" }) { Build-Architecture "ARM64" $Arm64BuildRoot $Configuration }
    }
}

function Convert-LicenseTextToRtf([string]$InputPath, [string]$OutputPath) {
    if (-not (Test-Path $GeneratedDir)) { New-Item -ItemType Directory -Path $GeneratedDir | Out-Null }
    $content = Get-Content -LiteralPath $InputPath -Raw
    $escaped = $content -replace '\\', '\\\\' -replace '\{', '\{' -replace '\}', '\}' -replace "`r`n", "\par`n" -replace "`n", "\par`n" -replace "`r", "\par`n"
    $rtf = "{\rtf1\ansi\deff0{\fonttbl{\f0 Arial;}}\viewkind4\uc1\pard\f0\fs20 " + $escaped + "}"
    Set-Content -LiteralPath $OutputPath -Value $rtf -Encoding ASCII
}

Check-AndBuildMissingArtifacts
Convert-LicenseTextToRtf -InputPath $LicenseTxtPath -OutputPath $LicenseRtfPath

function Find-WixExecutable {
    $candidates = @("C:\Program Files\WiX Toolset v7.0\bin\wix.exe", "C:\Program Files\WiX Toolset v4.0\bin\wix.exe")
    $cmd = Get-Command wix -ErrorAction SilentlyContinue
    if ($cmd) { $candidates += $cmd.Source }
    foreach ($c in $candidates | Select-Object -Unique) { if (Test-Path $c) { return $c } }
    return $null
}

function Invoke-WixBuild([string]$WixExe, [string]$OutDir, [string]$OutputName, [string]$X64BinDir, [string]$X86BinDir, [string]$Arm64BinDir, [string]$OpenCCDir) {
    $MsiPath = Join-Path $OutDir $OutputName

    Write-Host "Building MSI installer (zh-TW)..." -ForegroundColor Cyan
    # We use zh-TW as the primary culture for the installer UI.
    # We still provide both .wxl files to the build process.
    & $WixExe build -ext WixToolset.UI.wixext -ext WixToolset.Util.wixext `
        .\installer.wxs .\zh-TW.wxl .\en-US.wxl `
        -culture zh-TW -o $MsiPath `
        -b "X64BinDir=$X64BinDir" -b "X86BinDir=$X86BinDir" -b "Arm64BinDir=$Arm64BinDir" -b "OpenCCDir=$OpenCCDir"
    
    if ($LASTEXITCODE -ne 0) { throw "MSI build failed" }
}

$WixExe = Find-WixExecutable
if (-not $WixExe) { Write-Host "Error: WiX CLI not found." -ForegroundColor Red; exit 1 }

$OutDir = "dist"
if (-not (Test-Path $OutDir)) { New-Item -ItemType Directory -Path $OutDir | Out-Null }

Invoke-WixBuild $WixExe $OutDir $OutputName $X64BinDir $X86BinDir $Arm64BinDir $OpenCCDir
Write-Host "Successfully created MSI at: dist\$OutputName" -ForegroundColor Green
