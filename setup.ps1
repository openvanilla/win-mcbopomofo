# PowerShell script to install Win-McBopomofo

# Requires Admin privileges
if (!([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Write-Warning "Please run this script as Administrator."
    Exit
}

$DefaultInstallDir = "$env:ProgramFiles\McBopomofo"
$installDir = Read-Host "Enter installation directory [$DefaultInstallDir]"
if ([string]::IsNullOrWhiteSpace($installDir)) {
    $installDir = $DefaultInstallDir
}

# Ensure we have the necessary source files in 'dist' or 'build'
# For now, we assume files are in 'dist' as prepared by a build process.
$sourceDir = "$PSScriptRoot\dist"
if (!(Test-Path $sourceDir)) {
    Write-Error "Source directory '$sourceDir' not found. Please build the project first or ensure 'dist' folder exists."
    Exit
}

Write-Host "`n1. Stopping existing McBopomofo processes..."
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
        Write-Warning "Installation may fail if DLLs are locked. Please close the applications manually and try again."
        $choiceAbort = Read-Host "Abort installation? (Y/N)"
        if ($choiceAbort -eq 'Y' -or $choiceAbort -eq 'y') { Exit }
    }
}

Write-Host "`n3. Copying files to $installDir..."
if (!(Test-Path $installDir)) { New-Item -ItemType Directory -Path $installDir -Force }
if (!(Test-Path "$installDir\data")) { New-Item -ItemType Directory -Path "$installDir\data" -Force }

Copy-Item "$sourceDir\McBopomofoServer.exe" "$installDir\" -Force
Copy-Item "$sourceDir\McBopomofoConfig.exe" "$installDir\" -Force
Copy-Item "$sourceDir\McBopomofoTIP_x64.dll" "$installDir\" -Force -ErrorAction SilentlyContinue
Copy-Item "$sourceDir\McBopomofoTIP_x86.dll" "$installDir\" -Force -ErrorAction SilentlyContinue
Copy-Item "$sourceDir\McBopomofoTIP_arm64.dll" "$installDir\" -Force -ErrorAction SilentlyContinue
Copy-Item "$sourceDir\data\*" "$installDir\data\" -Recurse -Force

Write-Host "4. Granting AppContainer permissions..."
icacls "$installDir" /grant "ALL APPLICATION PACKAGES:(OI)(CI)(RX)" /T /Q

Write-Host "5. Registering TSF DLLs..."
if (Test-Path "$installDir\McBopomofoTIP_x64.dll") {
    Start-Process -FilePath "C:\Windows\System32\regsvr32.exe" -ArgumentList "/s `"$installDir\McBopomofoTIP_x64.dll`"" -Wait
}
if (Test-Path "$installDir\McBopomofoTIP_x86.dll") {
    Start-Process -FilePath "C:\Windows\SysWOW64\regsvr32.exe" -ArgumentList "/s `"$installDir\McBopomofoTIP_x86.dll`"" -Wait
}
if ((Test-Path "$installDir\McBopomofoTIP_arm64.dll") -and ($env:PROCESSOR_ARCHITECTURE -eq "ARM64" -or $env:PROCESSOR_ARCHITEW6432 -eq "ARM64")) {
    Start-Process -FilePath "C:\Windows\System32\regsvr32.exe" -ArgumentList "/s `"$installDir\McBopomofoTIP_arm64.dll`"" -Wait
}

Write-Host "6. Restarting TSF (ctfmon.exe)..."
Stop-Process -Name "ctfmon" -Force -ErrorAction SilentlyContinue
Start-Process "ctfmon.exe"
Start-Sleep -Seconds 1

Write-Host "7. Configuring auto-start in Registry..."
$RegistryPath = "HKCU:\Software\Microsoft\Windows\CurrentVersion\Run"
$Name = "McBopomofoServer"
$Value = "`"$installDir\McBopomofoServer.exe`""
Set-ItemProperty -Path $RegistryPath -Name $Name -Value $Value

Write-Host "8. Starting McBopomofoServer..."
# Note: McBopomofoServer defaults to looking for data/data.txt relative to its own path.
Start-Process -FilePath "$installDir\McBopomofoServer.exe" -WorkingDirectory "$installDir" -WindowStyle Hidden

Write-Host "`nInstallation complete!"
Write-Host "McBopomofo has been installed to $installDir"
Write-Host "The server will now run automatically on startup."
