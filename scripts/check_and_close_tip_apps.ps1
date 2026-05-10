# PowerShell script to check and close applications that have loaded McBopomofoTIP DLL
# This script is called from WiX installer as a CustomAction

param(
    [string]$Mode = "Check"  # "Check" or "Close"
)

$dllPattern = "McBopomofoTIP*.dll"
$ErrorActionPreference = "Stop"

function Find-ProcessesWithTIP {
    Write-Host "Scanning for processes that have loaded $dllPattern..."
    
    $processes = Get-Process -ErrorAction SilentlyContinue
    $lockedProcesses = @()
    
    foreach ($p in $processes) {
        try {
            $modules = $p.Modules | Select-Object -ExpandProperty ModuleName -ErrorAction SilentlyContinue
            if ($modules | Where-Object { $_ -like $dllPattern }) {
                Write-Host "Found: Process $($p.ProcessName) (PID: $($p.Id)) has loaded the DLL."
                $lockedProcesses += @{
                    ProcessName = $p.ProcessName
                    ProcessId = $p.Id
                    Process = $p
                }
            }
        }
        catch {
            # Ignore access denied errors for system processes
        }
    }
    
    return $lockedProcesses
}

function Show-WarningDialog {
    param([string[]]$ProcessNames)
    
    Add-Type -AssemblyName System.Windows.Forms
    $unique_names = $ProcessNames | Sort-Object -Unique
    $processes_list = $unique_names -join "`n"
    
    $message = @"
以下應用程式已載入 McBopomofo 輸入法 (McBopomofoTIP*.dll)：

$processes_list

我們需要關閉這些應用程式才能繼續安裝。

點擊「確定」讓我們為您關閉這些應用程式，
或點擊「取消」中止安裝。
"@
    
    $result = [System.Windows.Forms.MessageBox]::Show(
        $message, 
        "McBopomofo 安裝程式 - 需要關閉應用程式", 
        [System.Windows.Forms.MessageBoxButtons]::OKCancel, 
        [System.Windows.Forms.MessageBoxIcon]::Warning
    )
    
    return $result -eq [System.Windows.Forms.DialogResult]::OK
}

function Close-LockedProcesses {
    param([object[]]$LockedProcesses)
    
    $safeToKill = @(
        "notepad", "cmd", "powershell", "pwsh", "wordpad", 
        "chrome", "msedge", "firefox", "ApplicationFrameHost", 
        "dllhost", "explorer"
    )
    
    foreach ($item in $LockedProcesses) {
        $processName = $item.ProcessName
        $processId = $item.ProcessId
        
        try {
            if ($processName -eq "ctfmon") {
                Write-Host "Restarting ctfmon.exe..."
                Stop-Process -Id $processId -Force -ErrorAction SilentlyContinue
                Start-Sleep -Milliseconds 500
                Start-Process "ctfmon.exe" -ErrorAction SilentlyContinue
            }
            else {
                Write-Host "Closing $processName (PID: $processId)..."
                Stop-Process -Id $processId -Force -ErrorAction SilentlyContinue
            }
        }
        catch {
            Write-Host "Error closing process $processName : $_"
        }
    }
    
    Start-Sleep -Milliseconds 1000
}

# Main logic
try {
    $lockedProcesses = Find-ProcessesWithTIP
    
    if ($lockedProcesses.Count -gt 0) {
        Write-Host "Found $($lockedProcesses.Count) process(es) with McBopomofoTIP DLL loaded."
        
        if ($Mode -eq "Check") {
            $processNames = $lockedProcesses | ForEach-Object { $_.ProcessName }
            if (-not (Show-WarningDialog -ProcessNames $processNames)) {
                Write-Host "User cancelled installation."
                exit 1
            }
        }
        
        Close-LockedProcesses -LockedProcesses $lockedProcesses
        Write-Host "Successfully closed processes."
    }
    else {
        Write-Host "No processes found with McBopomofoTIP DLL loaded."
    }
    
    exit 0
}
catch {
    Write-Host "Error: $_"
    exit 1
}
