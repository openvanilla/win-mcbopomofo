# PowerShell script to uninstall Win-McBopomofo

# Requires Admin privileges
if (!([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Write-Warning "Please run this script as Administrator."
    Exit
}

$installDir = "$PSScriptRoot\dist"

Write-Host "1. Stopping processes..."
Stop-Process -Name "McBopomofoServer" -Force -ErrorAction SilentlyContinue
Stop-Process -Name "McBopomofoConfig" -Force -ErrorAction SilentlyContinue
& "$PSScriptRoot\close_ime_apps.ps1"
Start-Sleep -Seconds 1

Write-Host "2. Unregistering TSF DLLs..."
if (Test-Path "$installDir\McBopomofoTIP_x64.dll") {
    Start-Process -FilePath "C:\Windows\System32\regsvr32.exe" -ArgumentList "/u /s `"$installDir\McBopomofoTIP_x64.dll`"" -Wait
}
if (Test-Path "$installDir\McBopomofoTIP_x86.dll") {
    Start-Process -FilePath "C:\Windows\SysWOW64\regsvr32.exe" -ArgumentList "/u /s `"$installDir\McBopomofoTIP_x86.dll`"" -Wait
}
if ((Test-Path "$installDir\McBopomofoTIP_arm64.dll") -and $env:PROCESSOR_ARCHITECTURE -eq "ARM64") {
    Start-Process -FilePath "C:\Windows\System32\regsvr32.exe" -ArgumentList "/u /s `"$installDir\McBopomofoTIP_arm64.dll`"" -Wait
}

Write-Host "3. Restarting TSF..."
Stop-Process -Name "ctfmon" -Force -ErrorAction SilentlyContinue
Start-Process "ctfmon.exe"

Write-Host "`nWin-McBopomofo has been unregistered."
Write-Host "Note: Files in '$installDir' were NOT deleted. You can delete them manually if desired."
