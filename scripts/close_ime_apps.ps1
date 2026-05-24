# PowerShell script to close all applications locking the McBopomofo DLL
# Usage: .\close_ime_apps.ps1 [-ShowUI] [-SilentFail]

param(
    [switch]$ShowUI = $false,
    [switch]$SilentFail = $false
)

$dllPattern = "McBopomofoTIP*.dll"
$dllName = "McBopomofoTIP_v2.dll"
Write-Host "Finding processes that have loaded $dllPattern..."

# Common processes we might want to kill, but we need to be careful with explorer.exe
$safeToKill = @("notepad", "cmd", "powershell", "pwsh", "wordpad", "chrome", "msedge", "firefox", "ApplicationFrameHost", "dllhost")

$processes = Get-Process -ErrorAction SilentlyContinue
$lockedProcesses = @()

foreach ($p in $processes) {
    try {
        $modules = $p.Modules | Select-Object -ExpandProperty ModuleName -ErrorAction SilentlyContinue
        if ($modules | Where-Object { $_ -like $dllPattern }) {
            Write-Host "Process $($p.ProcessName) (PID: $($p.Id)) has loaded the DLL."
            $lockedProcesses += @{
                ProcessName = $p.ProcessName
                ProcessId = $p.Id
                Process = $p
            }
        }
    }
    catch {
        # Ignore access denied errors for system processes we can't inspect
    }
}

if ($lockedProcesses.Count -gt 0) {
    if ($ShowUI) {
        # Show UI prompt to user
        Add-Type -AssemblyName System.Windows.Forms
        $processes_list = $lockedProcesses | ForEach-Object { $_.ProcessName } | Sort-Object -Unique
        $message = "The following applications have loaded the McBopomofo input method:`n`n" + ($processes_list -join "`n") + "`n`nThese applications must be closed to complete installation.`n`nClick OK to continue, or Cancel to abort installation."
        $result = [System.Windows.Forms.MessageBox]::Show($message, "McBopomofo Installer", [System.Windows.Forms.MessageBoxButtons]::OKCancel, [System.Windows.Forms.MessageBoxIcon]::Warning)
        
        if ($result -eq [System.Windows.Forms.DialogResult]::Cancel) {
            Write-Host "User cancelled installation."
            if (-not $SilentFail) {
                exit 1
            }
            exit 0
        }
    }
    
    foreach ($item in $lockedProcesses) {
        $processName = $item.ProcessName
        $processId = $item.ProcessId
        
        try {
            if ($processName -eq "ctfmon") {
                Write-Host "Restarting ctfmon.exe..."
                Stop-Process -Id $processId -Force -ErrorAction SilentlyContinue
                Start-Sleep -Milliseconds 500
                Start-Process "ctfmon.exe"
            }
            elseif ($safeToKill -contains $processName) {
                Write-Host "Closing $($processName)..."
                Stop-Process -Id $processId -Force -ErrorAction SilentlyContinue
            }
            else {
                Write-Host "Warning: Process $processName is locking the DLL but is not in the safe-to-kill list."
            }
        }
        catch {
            Write-Host "Error closing process $processName : $_"
        }
    }
    
    Start-Sleep -Milliseconds 1000
}

Write-Host "Done."
