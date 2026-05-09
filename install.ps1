# PowerShell script to build, install, and start the Win-McBopomofo environment

param(
    [ValidateSet("Debug", "Release")]
    [string]$BuildType = "Debug"
)

$scriptsDir = Join-Path $PSScriptRoot "scripts"

# Requires Admin privileges
if (!([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Write-Warning "Please run this script as Administrator."
    Exit
}

$installDir = "$PSScriptRoot\dist"
if (!(Test-Path $installDir)) { New-Item -ItemType Directory -Path $installDir }
if (!(Test-Path "$installDir\data")) { New-Item -ItemType Directory -Path "$installDir\data" }

Write-Host "1. Stopping existing instances and cleaning up locks..."
& (Join-Path $scriptsDir "close_ime_apps.ps1")
Stop-Process -Name "McBopomofoServer" -Force -ErrorAction SilentlyContinue
Stop-Process -Name "McBopomofoConfig" -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 1

Write-Host "2. Building x64 (64-bit)..."
cmake -S . -B build_x64 -A x64
cmake --build build_x64 --config $BuildType --target Dictionaries McBopomofoTIP McBopomofoServer McBopomofoConfig

Write-Host "3. Building Win32 (32-bit)..."
cmake -S . -B build_x86 -A Win32
cmake --build build_x86 --config $BuildType --target McBopomofoTIP

Write-Host "4. Building ARM64..."
cmake -S . -B build_arm64 -A ARM64
cmake --build build_arm64 --config $BuildType --target McBopomofoTIP

Write-Host "5. Staging files to 'dist' folder..."
Copy-Item "build_x64\bin\$BuildType\McBopomofoServer.exe" "$installDir\"
Copy-Item "build_x64\bin\$BuildType\McBopomofoConfig.exe" "$installDir\"
Copy-Item "build_x64\bin\$BuildType\McBopomofoTIP_v2.dll" "$installDir\McBopomofoTIP_x64.dll"
Copy-Item "build_x86\bin\$BuildType\McBopomofoTIP_v2.dll" "$installDir\McBopomofoTIP_x86.dll"
Copy-Item "build_arm64\bin\$BuildType\McBopomofoTIP_v2.dll" "$installDir\McBopomofoTIP_arm64.dll"
Copy-Item "data\data.txt" "$installDir\data\"
Copy-Item "data\data-plain-bpmf.txt" "$installDir\data\"
Copy-Item "data\associated-phrases-v2.txt" "$installDir\data\"
Copy-Item "data\dictionary_service.json" "$installDir\data\"
Copy-Item "data\bpmfvs-variants.txt" "$installDir\data\"
Copy-Item "data\bpmfvs-pua.txt" "$installDir\data\"

if (!(Test-Path "$installDir\data\opencc")) { New-Item -ItemType Directory -Path "$installDir\data\opencc" }
Copy-Item "third_party\OpenCC\data\config\tw2s.json" "$installDir\data\opencc\" -Force

$openccBuildDir = Join-Path $PSScriptRoot "build_x64\third_party\OpenCC\data"
$openccRequiredFiles = @(
    "TSPhrases.ocd2",
    "TSCharacters.ocd2",
    "TWVariantsRev.ocd2",
    "TWVariantsRevPhrases.ocd2"
)

foreach ($file in $openccRequiredFiles) {
    $source = Join-Path $openccBuildDir $file
    if (!(Test-Path $source)) {
        Write-Error "Required OpenCC dictionary '$source' not found. Build output is incomplete."
        Exit 1
    }
    Copy-Item $source "$installDir\data\opencc\" -Force
}

Write-Host "6. Granting AppContainer (UWP) permissions to dist folder..."
icacls "$installDir" /grant "ALL APPLICATION PACKAGES:(OI)(CI)(RX)" /T /Q

Write-Host "7. Registering TSF DLLs..."
Start-Process -FilePath "C:\Windows\System32\regsvr32.exe" -ArgumentList "/s `"$installDir\McBopomofoTIP_x64.dll`"" -Wait
Start-Process -FilePath "C:\Windows\SysWOW64\regsvr32.exe" -ArgumentList "/s `"$installDir\McBopomofoTIP_x86.dll`"" -Wait
if ($env:PROCESSOR_ARCHITECTURE -eq "ARM64") {
    Start-Process -FilePath "C:\Windows\System32\regsvr32.exe" -ArgumentList "/s `"$installDir\McBopomofoTIP_arm64.dll`"" -Wait
} else {
    Write-Host "Skipping ARM64 DLL registration on non-ARM64 host."
}

Write-Host "8. Restarting TSF..."
Stop-Process -Name "ctfmon" -Force -ErrorAction SilentlyContinue
Start-Process "ctfmon.exe"
Start-Sleep -Seconds 1

Write-Host "9. Starting McBopomofoServer..."
$serverPath = "$installDir\McBopomofoServer.exe"
$dataPath = "$installDir\data\data.txt"
Start-Process -FilePath $serverPath -ArgumentList "`"$dataPath`"" -WorkingDirectory "$installDir" -WindowStyle Hidden

Write-Host "`nDone! Win-McBopomofo is installed in: $installDir"
Write-Host "You can launch the settings tool from: $installDir\McBopomofoConfig.exe"
