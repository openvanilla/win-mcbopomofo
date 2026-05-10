# build_msi.ps1
# Script to build the MSI installer using WiX v4
# This script automatically builds all required binaries first
#
# Usage:
#   .\build_msi.ps1                              # Build all architectures and MSI
#   .\build_msi.ps1 -Configuration Debug         # Build in Debug configuration
#   .\build_msi.ps1 -SkipBuild                   # Skip binary build, only create MSI
#   .\build_msi.ps1 -OutputName "Custom.msi"     # Custom output MSI name
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
    
    # On 32-bit OS, PROCESSOR_ARCHITEW6432 is not set
    if ($null -eq $processorArchW6432) {
        # Running on native 32-bit or 64-bit process
        switch ($processorArchitecture) {
            "AMD64" { return "x64" }
            "ARM64" { return "ARM64" }
            "x86" { return "x86" }
            default { return $processorArchitecture }
        }
    }
    else {
        # 32-bit process on 64-bit OS
        switch ($processorArchW6432) {
            "AMD64" { return "x64" }
            "ARM64" { return "ARM64" }
            default { return "x64" }
        }
    }
}

$CurrentPlatform = Get-CurrentPlatform
Write-Host "Detected current platform: $CurrentPlatform" -ForegroundColor Cyan

# Determine if we need to skip OpenCC dict building
function Should-SkipOpenCCDict {
    param([string]$TargetArchitecture, [string]$CurrentPlatform)
    
    # We can build dict if the target matches current platform
    # Otherwise, skip it to avoid cross-compilation issues
    return ($TargetArchitecture -ne $CurrentPlatform)
}

function Get-CMakeCacheValue {
    param(
        [string]$CachePath,
        [string]$VariableName
    )

    if (-not (Test-Path $CachePath)) {
        return $null
    }

    $match = Select-String -Path $CachePath -Pattern "^$([regex]::Escape($VariableName)):.*=(.*)$" -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($null -eq $match) {
        return $null
    }

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
    
    # Create build directory if it doesn't exist
    if (-not (Test-Path $BuildRoot)) {
        New-Item -ItemType Directory -Path $BuildRoot | Out-Null
    }
    
    Push-Location $BuildRoot
    try {
        # Determine if we should skip OpenCC dict building for this target
        $skipOpenCCDict = Should-SkipOpenCCDict -TargetArchitecture $Architecture -CurrentPlatform $CurrentPlatform
        $skipOpenCCDictFlag = if ($skipOpenCCDict) { "-DSKIP_OPENCC_DICT=ON" } else { "-DSKIP_OPENCC_DICT=OFF" }
        $cachePath = Join-Path $BuildRoot "CMakeCache.txt"
        $cachedSkipOpenCCDict = Get-CMakeCacheValue -CachePath $cachePath -VariableName "SKIP_OPENCC_DICT"
        $needsConfigure = $true

        if ($cachedSkipOpenCCDict -ne $null) {
            $cachedSkipOpenCCDict = $cachedSkipOpenCCDict.Trim()
            if (($skipOpenCCDict -and $cachedSkipOpenCCDict -eq "ON") -or (-not $skipOpenCCDict -and $cachedSkipOpenCCDict -eq "OFF")) {
                $needsConfigure = $false
            }
        }
        
        if ($needsConfigure) {
            if ($skipOpenCCDict) {
                Write-Host "Skipping OpenCC dictionary building for $Architecture (current platform: $CurrentPlatform)" -ForegroundColor Yellow
            }

            Write-Host "Running CMake configure for $Architecture..." -ForegroundColor Yellow

            $cmakeArgs = @(
                $skipOpenCCDictFlag,
                "-DCMAKE_BUILD_TYPE=$Configuration"
            )

            if ($Architecture -eq "ARM64") {
                cmake -A ARM64 @cmakeArgs ..
            } elseif ($Architecture -eq "x86") {
                cmake -A Win32 @cmakeArgs ..
            } else {
                cmake -A x64 @cmakeArgs ..
            }

            if ($LASTEXITCODE -ne 0) {
                throw "CMake configure failed for $Architecture"
            }
        } else {
            Write-Host "Reusing existing CMake cache for $Architecture." -ForegroundColor Cyan
        }
        
        # Run CMake build step
        Write-Host "Building $Architecture binary files..." -ForegroundColor Yellow
        if ($Architecture -eq "x64") {
            cmake --build . --config $Configuration --target third_party/OpenCC/data/Dictionaries
            if ($LASTEXITCODE -ne 0) {
                throw "CMake dictionary build failed for $Architecture"
            }
        }

        cmake --build . --config $Configuration --target McBopomofoTIP McBopomofoServer McBopomofoConfig
        
        if ($LASTEXITCODE -ne 0) {
            throw "CMake build failed for $Architecture"
        }
        
        Write-Host "Successfully built $Architecture architecture" -ForegroundColor Green
    }
    finally {
        Pop-Location
    }
}

function Check-AndBuildMissingArtifacts {
    $missingArtifacts = @()
    
    foreach ($artifact in $requiredArtifacts) {
        if (-not (Test-Path $artifact)) {
            $missingArtifacts += $artifact
        }
    }
    
    if ($missingArtifacts.Count -gt 0) {
        Write-Host "Found missing artifacts:" -ForegroundColor Yellow
        foreach ($artifact in $missingArtifacts) {
            Write-Host "  - $artifact" -ForegroundColor Yellow
        }
        
        if ($SkipBuild) {
            Write-Host "Error: Required artifacts are missing and -SkipBuild flag is set." -ForegroundColor Red
            exit 1
        }
        
        Write-Host "Building missing artifacts..." -ForegroundColor Cyan
        
        # Determine which architectures need to be built
        $needsX64 = $missingArtifacts | Where-Object { $_ -like "*$X64BuildRoot*" }
        $needsX86 = $missingArtifacts | Where-Object { $_ -like "*$X86BuildRoot*" }
        $needsArm64 = $missingArtifacts | Where-Object { $_ -like "*$Arm64BuildRoot*" }
        
        if ($needsX64) { Build-Architecture "x64" $X64BuildRoot $Configuration }
        if ($needsX86) { Build-Architecture "x86" $X86BuildRoot $Configuration }
        if ($needsArm64) { Build-Architecture "ARM64" $Arm64BuildRoot $Configuration }
        
        # Verify all artifacts now exist
        $stillMissing = @()
        foreach ($artifact in $requiredArtifacts) {
            if (-not (Test-Path $artifact)) {
                $stillMissing += $artifact
            }
        }
        
        if ($stillMissing.Count -gt 0) {
            Write-Host "Error: Build completed but the following artifacts are still missing:" -ForegroundColor Red
            foreach ($artifact in $stillMissing) {
                Write-Host "  - $artifact" -ForegroundColor Red
            }
            exit 1
        }
    }
    else {
        Write-Host "All required artifacts are present." -ForegroundColor Green
    }
}

function Convert-LicenseTextToRtf([string]$InputPath, [string]$OutputPath) {
    if (-not (Test-Path $InputPath)) {
        throw "License file not found: $InputPath"
    }

    if (-not (Test-Path $GeneratedDir)) {
        New-Item -ItemType Directory -Path $GeneratedDir | Out-Null
    }

    $content = Get-Content -LiteralPath $InputPath -Raw
    $escaped = $content `
        -replace '\\', '\\\\' `
        -replace '\{', '\{' `
        -replace '\}', '\}' `
        -replace "`r`n", "\par`n" `
        -replace "`n", "\par`n" `
        -replace "`r", "\par`n"

    $rtf = "{\rtf1\ansi\deff0{\fonttbl{\f0 Arial;}}\viewkind4\uc1\pard\f0\fs20 " + $escaped + "}"
    Set-Content -LiteralPath $OutputPath -Value $rtf -Encoding ASCII
}

# Check for CMake
$cmake = Get-Command cmake -ErrorAction SilentlyContinue
if (-not $cmake) {
    Write-Host "Error: CMake is not installed or not in PATH." -ForegroundColor Red
    exit 1
}

Write-Host "Using CMake: $($cmake.Source)" -ForegroundColor Cyan

# Build missing artifacts
Check-AndBuildMissingArtifacts
Convert-LicenseTextToRtf -InputPath $LicenseTxtPath -OutputPath $LicenseRtfPath

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

function Invoke-Wix4Build([string]$WixExe, [string]$MsiPath, [string]$X64BinDir, [string]$X86BinDir, [string]$Arm64BinDir, [string]$OpenCCDir) {
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
        .\zh-TW.wxl `
        .\en-US.wxl `
        -cultures zh-TW `
        -o $MsiPath `
        -b "X64BinDir=$X64BinDir" `
        -b "X86BinDir=$X86BinDir" `
        -b "Arm64BinDir=$Arm64BinDir" `
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

Write-Host "Building MSI using WiX $WixVersionText..." -ForegroundColor Cyan

# Ensure output directory exists
$OutDir = "dist"
if (-not (Test-Path $OutDir)) {
    New-Item -ItemType Directory -Path $OutDir | Out-Null
}

$MsiPath = "$OutDir\$OutputName"

if ($WixMajorVersion -eq 4) {
    try {
        Invoke-Wix4Build -WixExe $WixExe -MsiPath $MsiPath -X64BinDir $X64BinDir -X86BinDir $X86BinDir -Arm64BinDir $Arm64BinDir -OpenCCDir $OpenCCDir
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
        .\zh-TW.wxl `
        .\en-US.wxl `
        -cultures zh-TW `
        -o $MsiPath `
        -b "X64BinDir=$X64BinDir" `
        -b "X86BinDir=$X86BinDir" `
        -b "Arm64BinDir=$Arm64BinDir" `
        -b "OpenCCDir=$OpenCCDir"
}

if ($LASTEXITCODE -eq 0) {
    Write-Host "Successfully created MSI at: $MsiPath" -ForegroundColor Green
} else {
    Write-Host "Failed to build MSI. See output for details." -ForegroundColor Red
    exit $LASTEXITCODE
}
