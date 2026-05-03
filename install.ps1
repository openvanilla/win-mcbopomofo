# PowerShell script to build, install, and start the Win-McBopomofo environment

# Requires Admin privileges
if (!([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Write-Warning "Please run this script as Administrator."
    Exit
}

$installDir = "$PSScriptRoot\dist"
if (!(Test-Path $installDir)) { New-Item -ItemType Directory -Path $installDir }
if (!(Test-Path "$installDir\data")) { New-Item -ItemType Directory -Path "$installDir\data" }

Write-Host "1. Stopping existing instances and cleaning up locks..."
& "$PSScriptRoot\close_ime_apps.ps1"
Stop-Process -Name "McBopomofoServer" -Force -ErrorAction SilentlyContinue
Stop-Process -Name "McBopomofoConfig" -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 1

Write-Host "2. Building x64 (64-bit)..."
cmake -S . -B build_x64 -A x64
cmake --build build_x64 --config Release --target McBopomofoTIP McBopomofoServer McBopomofoConfig

Write-Host "3. Building Win32 (32-bit)..."
cmake -S . -B build_x86 -A Win32
cmake --build build_x86 --config Release --target McBopomofoTIP

Write-Host "4. Staging files to 'dist' folder..."
Copy-Item "build_x64\bin\Release\McBopomofoServer.exe" "$installDir\"
Copy-Item "build_x64\bin\Release\McBopomofoConfig.exe" "$installDir\"
Copy-Item "build_x64\bin\Release\McBopomofoTIP_v2.dll" "$installDir\McBopomofoTIP_x64.dll"
Copy-Item "build_x86\bin\Release\McBopomofoTIP_v2.dll" "$installDir\McBopomofoTIP_x86.dll"
Copy-Item "data\data.txt" "$installDir\data\"
Copy-Item "data\associated-phrases-v2.txt" "$installDir\data\"

Write-Host "5. Granting AppContainer (UWP) permissions to dist folder..."
icacls "$installDir" /grant "ALL APPLICATION PACKAGES:(OI)(CI)(RX)" /T /Q

Write-Host "6. Registering TSF DLLs..."
Start-Process -FilePath "C:\Windows\System32\regsvr32.exe" -ArgumentList "/s `"$installDir\McBopomofoTIP_x64.dll`"" -Wait
Start-Process -FilePath "C:\Windows\SysWOW64\regsvr32.exe" -ArgumentList "/s `"$installDir\McBopomofoTIP_x86.dll`"" -Wait

Write-Host "7. Restarting TSF..."
Stop-Process -Name "ctfmon" -Force -ErrorAction SilentlyContinue
Start-Process "ctfmon.exe"
Start-Sleep -Seconds 1

Write-Host "8. Starting McBopomofoServer..."
$serverPath = "$installDir\McBopomofoServer.exe"
$dataPath = "$installDir\data\data.txt"
Start-Process -FilePath $serverPath -ArgumentList "`"$dataPath`"" -WorkingDirectory "$installDir" -WindowStyle Hidden

Write-Host "`nDone! Win-McBopomofo is installed in: $installDir"
Write-Host "You can launch the settings tool from: $installDir\McBopomofoConfig.exe"
