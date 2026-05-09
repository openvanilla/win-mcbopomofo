# PowerShell script to close all applications locking the McBopomofo DLL

$dllName = "McBopomofoTIP_v2.dll"
Write-Host "Finding processes that have loaded $dllName..."

# Common processes we might want to kill, but we need to be careful with explorer.exe
$safeToKill = @("notepad", "cmd", "powershell", "pwsh", "wordpad", "chrome", "msedge", "firefox", "ApplicationFrameHost", "dllhost")

$processes = Get-Process -ErrorAction SilentlyContinue

foreach ($p in $processes) {
    try {
        $modules = $p.Modules | Select-Object -ExpandProperty ModuleName -ErrorAction SilentlyContinue
        if ($modules -contains $dllName) {
            Write-Host "Process $($p.ProcessName) (PID: $($p.Id)) has loaded the DLL."
            
            if ($p.ProcessName -eq "ctfmon") {
                Write-Host "Restarting ctfmon.exe..."
                Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue
                Start-Process "ctfmon.exe"
            }
            elseif ($safeToKill -contains $p.ProcessName) {
                Write-Host "Killing $($p.ProcessName)..."
                Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue
            }
            else {
                Write-Host "Warning: Process $($p.ProcessName) is locking the DLL but is not in the safe-to-kill list. You may need to close it manually."
            }
        }
    }
    catch {
        # Ignore access denied errors for system processes we can't inspect
    }
}

Write-Host "Done."
