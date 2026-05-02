# PowerShell script to build, register, and start the Win-McBopomofo environment for debugging

# Requires Admin privileges to register the COM DLLs and modify ACLs
if (!([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Write-Warning "Please run this script as Administrator to register the COM DLLs and grant UWP folder permissions."
    Exit
}

Write-Host "1. Stopping existing McBopomofoServer instances and cleaning up locks..."
Stop-Process -Name "McBopomofoServer" -Force -ErrorAction SilentlyContinue
Stop-Process -Name "dllhost" -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 1

Write-Host "2. Building x64 (64-bit) Architecture..."
& "C:\Program Files\CMake\bin\cmake.exe" -S . -B build_x64 -A x64
& "C:\Program Files\CMake\bin\cmake.exe" --build build_x64 --target McBopomofoTIP --config Release
& "C:\Program Files\CMake\bin\cmake.exe" --build build_x64 --target McBopomofoServer --config Release

Write-Host "3. Building Win32 (32-bit) Architecture..."
& "C:\Program Files\CMake\bin\cmake.exe" -S . -B build_x86 -A Win32
& "C:\Program Files\CMake\bin\cmake.exe" --build build_x86 --target McBopomofoTIP --config Release

Write-Host "4. Granting AppContainer (UWP) read/execute permissions to build folders..."
# Without this, modern Notepad and Edge (which run in isolated AppContainers) CANNOT read the DLL from your user folder!
icacls "build_x64\bin\Release" /grant "ALL APPLICATION PACKAGES:(OI)(CI)(RX)" /T /Q
icacls "build_x86\bin\Release" /grant "ALL APPLICATION PACKAGES:(OI)(CI)(RX)" /T /Q

Write-Host "5. Registering 64-bit and 32-bit TSF DLLs..."
$dll64 = Resolve-Path "build_x64\bin\Release\McBopomofoTIP_v2.dll"
$dll32 = Resolve-Path "build_x86\bin\Release\McBopomofoTIP_v2.dll"

if ($dll64) {
    Start-Process -FilePath "C:\Windows\System32\regsvr32.exe" -ArgumentList "/s `"$dll64`"" -Wait
    Write-Host "Registered 64-bit DLL: $dll64"
}
if ($dll32) {
    Start-Process -FilePath "C:\Windows\SysWOW64\regsvr32.exe" -ArgumentList "/s `"$dll32`"" -Wait
    Write-Host "Registered 32-bit DLL: $dll32"
}

Write-Host "6. Restarting Text Services Framework (ctfmon.exe)..."
Stop-Process -Name "ctfmon" -Force -ErrorAction SilentlyContinue
Start-Process "ctfmon.exe"
Start-Sleep -Seconds 1

Write-Host "7. Starting McBopomofoServer (x64) daemon..."
$serverPath = Resolve-Path "build_x64\bin\Release\McBopomofoServer.exe"
if ($serverPath) {
    # Start it in the background
    Start-Process -FilePath $serverPath -ArgumentList "data/data.txt" -WindowStyle Hidden
    Write-Host "Started McBopomofoServer in background."
} else {
    Write-Error "Could not find McBopomofoServer.exe"
}

Write-Host "Done! You can now test the input method in Notepad (64-bit) or other apps."
Write-Host "To uninstall, run: regsvr32.exe /u `"$dll64`" and regsvr32.exe /u `"$dll32`""
