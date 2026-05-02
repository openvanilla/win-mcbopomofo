# PowerShell script to build, register, and start the Win-McBopomofo environment for debugging

# Requires Admin privileges to register the COM DLL
if (!([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Write-Warning "Please run this script as Administrator to register the COM DLL."
    Write-Warning "If you only want to build or test the server, you can ignore this."
    # We do not automatically elevate because it can break the shell session context.
}

Write-Host "1. Building McBopomofoTIP and McBopomofoServer..."
& "C:\Program Files\CMake\bin\cmake.exe" --build build --target McBopomofoTIP --config Debug
& "C:\Program Files\CMake\bin\cmake.exe" --build build --target McBopomofoServer --config Debug

Write-Host "2. Stopping existing McBopomofoServer instances..."
Stop-Process -Name "McBopomofoServer" -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 1

Write-Host "3. Registering McBopomofoTIP.dll..."
$dllPath = Resolve-Path "build\bin\Debug\McBopomofoTIP.dll"
if ($dllPath) {
    # /s for silent, but let's see output for debugging
    Start-Process -FilePath "regsvr32.exe" -ArgumentList "`"$dllPath`"" -Wait
    Write-Host "Registered TSF DLL: $dllPath"
} else {
    Write-Error "Could not find McBopomofoTIP.dll"
}

Write-Host "4. Starting McBopomofoServer daemon..."
$serverPath = Resolve-Path "build\bin\Debug\McBopomofoServer.exe"
if ($serverPath) {
    # Start it in the background
    Start-Process -FilePath $serverPath -ArgumentList "data/data.txt" -WindowStyle Hidden
    Write-Host "Started McBopomofoServer in background."
} else {
    Write-Error "Could not find McBopomofoServer.exe"
}

Write-Host "Done! You can now test the input method in Notepad or any other app."
Write-Host "To uninstall, run: regsvr32.exe /u `"$dllPath`""
