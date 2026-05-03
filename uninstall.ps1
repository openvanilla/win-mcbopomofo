# PowerShell script to uninstall Win-McBopomofo

# Requires Admin privileges
if (!([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Write-Warning "Please run this script as Administrator."
    Exit
}

$DefaultInstallDir = "$env:ProgramFiles\McBopomofo"
$installDir = Read-Host "Enter installation directory to uninstall [$DefaultInstallDir]"
if ([string]::IsNullOrWhiteSpace($installDir)) {
    $installDir = $DefaultInstallDir
}

if (!(Test-Path $installDir)) {
    Write-Warning "Installation directory '$installDir' not found."
}

Write-Host "`n1. Stopping McBopomofo processes..."
Stop-Process -Name "McBopomofoServer" -Force -ErrorAction SilentlyContinue
Stop-Process -Name "McBopomofoConfig" -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 1

Write-Host "2. Checking for locked DLLs..."
$dllNames = @("McBopomofoTIP_x64.dll", "McBopomofoTIP_x86.dll", "McBopomofoTIP_arm64.dll", "McBopomofoTIP_v2.dll")
$processesToKill = @()
$ctfmonFound = $false

$processes = Get-Process -ErrorAction SilentlyContinue
foreach ($p in $processes) {
    try {
        $modules = $p.Modules | Select-Object -ExpandProperty ModuleName -ErrorAction SilentlyContinue
        foreach ($dll in $dllNames) {
            if ($modules -contains $dll) {
                if ($p.ProcessName -eq "ctfmon") {
                    $ctfmonFound = $true
                } else {
                    $processesToKill += $p
                }
                break
            }
        }
    } catch {}
}

if ($ctfmonFound) {
    Write-Host "Restarting ctfmon.exe..."
    Stop-Process -Name "ctfmon" -Force -ErrorAction SilentlyContinue
    Start-Process "ctfmon.exe"
}

if ($processesToKill.Count -gt 0) {
    Write-Host "`nThe following processes are locking McBopomofo DLLs:" -ForegroundColor Yellow
    foreach ($p in $processesToKill) {
        Write-Host " - $($p.ProcessName) (PID: $($p.Id))"
    }
    $choice = Read-Host "Would you like to try and close these processes? (Y/N)"
    if ($choice -eq 'Y' -or $choice -eq 'y') {
        foreach ($p in $processesToKill) {
            Write-Host "Stopping $($p.ProcessName)..."
            Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue
        }
    } else {
        Write-Warning "Unregistration may fail if DLLs are locked. Please close the applications manually and try again."
        $choiceAbort = Read-Host "Abort uninstallation? (Y/N)"
        if ($choiceAbort -eq 'Y' -or $choiceAbort -eq 'y') { Exit }
    }
}

Write-Host "`n3. Unregistering TSF DLLs..."
if (Test-Path "$installDir\McBopomofoTIP_x64.dll") {
    Start-Process -FilePath "C:\Windows\System32\regsvr32.exe" -ArgumentList "/u /s `"$installDir\McBopomofoTIP_x64.dll`"" -Wait
}
if (Test-Path "$installDir\McBopomofoTIP_x86.dll") {
    Start-Process -FilePath "C:\Windows\SysWOW64\regsvr32.exe" -ArgumentList "/u /s `"$installDir\McBopomofoTIP_x86.dll`"" -Wait
}
if ((Test-Path "$installDir\McBopomofoTIP_arm64.dll") -and ($env:PROCESSOR_ARCHITECTURE -eq "ARM64" -or $env:PROCESSOR_ARCHITEW6432 -eq "ARM64")) {
    Start-Process -FilePath "C:\Windows\System32\regsvr32.exe" -ArgumentList "/u /s `"$installDir\McBopomofoTIP_arm64.dll`"" -Wait
}

Write-Host "4. Removing auto-start from Registry..."
$RegistryPath = "HKCU:\Software\Microsoft\Windows\CurrentVersion\Run"
$Name = "McBopomofoServer"
Remove-ItemProperty -Path $RegistryPath -Name $Name -ErrorAction SilentlyContinue

Write-Host "`nWin-McBopomofo has been unregistered."

$choiceDelete = Read-Host "Would you like to delete the installation directory '$installDir'? (Y/N)"
if ($choiceDelete -eq 'Y' -or $choiceDelete -eq 'y') {
    Write-Host "Deleting $installDir..."
    Remove-Item -Path $installDir -Recurse -Force -ErrorAction SilentlyContinue
    Write-Host "Done."
} else {
    Write-Host "Files in '$installDir' were NOT deleted."
}
