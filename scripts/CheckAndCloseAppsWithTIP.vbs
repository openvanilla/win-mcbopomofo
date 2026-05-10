Dim objShell, objFSO, objExec, strOutput, arrLines, i, msgResult, processNames
Dim found, processName, uniqueProcesses, count, msg

Set objShell = CreateObject("WScript.Shell")
Set objFSO = CreateObject("Scripting.FileSystemObject")

' Use tasklist to find processes that have loaded McBopomofoTIP DLL
On Error Resume Next
Set objExec = objShell.Exec("tasklist /m McBopomofoTIP*.dll 2>nul")

If Err.Number <> 0 Then
    ' Command failed, exit silently
    WScript.Quit 0
End If

On Error Goto 0

strOutput = objExec.StdOut.ReadAll()
arrLines = Split(strOutput, vbCRLF)

' Parse output and build unique process name list
Dim arrUniqueProcesses()
ReDim arrUniqueProcesses(0)
count = 0

For i = 0 To UBound(arrLines)
    Dim line
    line = Trim(arrLines(i))
    
    If Len(line) > 0 And InStr(1, line, ".exe", 1) > 0 Then
        ' Extract process name from tasklist output
        Dim parts
        parts = Split(line, " ")
        If UBound(parts) >= 0 Then
            processName = LCase(parts(0))
            
            ' Check if this process is already in our list
            found = False
            Dim j
            For j = 0 To count - 1
                If LCase(arrUniqueProcesses(j)) = processName Then
                    found = True
                    Exit For
                End If
            Next
            
            If Not found Then
                If count = 0 Then
                    arrUniqueProcesses(0) = processName
                Else
                    ReDim Preserve arrUniqueProcesses(count)
                    arrUniqueProcesses(count) = processName
                End If
                count = count + 1
            End If
        End If
    End If
Next

' If processes found, show warning dialog and close them
If count > 0 Then
    ' Build message string
    msg = "The following applications have loaded McBopomofoTIP*.dll:" & vbCRLF & vbCRLF
    
    For i = 0 To count - 1
        msg = msg & arrUniqueProcesses(i) & vbCRLF
    Next
    
    msg = msg & vbCRLF & "These applications must be closed before installation can continue." & vbCRLF & vbCRLF
    msg = msg & "Click OK to close them automatically, or Cancel to stop the installation."
    
    ' Show warning dialog
    msgResult = MsgBox(msg, vbExclamation + vbOKCancel, "McBopomofo Installer - Close Applications")
    
    If msgResult = vbCancel Then
        WScript.Quit 1
    End If
    
    ' Close the processes
    For i = 0 To count - 1
        objShell.Run "taskkill.exe /f /im " & arrUniqueProcesses(i), 0, False
    Next
    
    ' Special handling for ctfmon - restart it
    objShell.Run "taskkill.exe /f /im ctfmon.exe", 0, False
    WScript.Sleep 500
    objShell.Run "ctfmon.exe", 0, False
    
    WScript.Sleep 1000
End If

WScript.Quit 0
