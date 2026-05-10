Sub Main()
    Dim objShell, objExec, strOutput, arrLines, i, msgResult
    Dim found, processName, count, msg
    Dim arrUniqueProcesses()

    Set objShell = CreateObject("WScript.Shell")

    On Error Resume Next
    Set objExec = objShell.Exec("tasklist /m McBopomofoTIP*.dll 2>nul")
    If Err.Number <> 0 Then
        Err.Clear
        Exit Sub
    End If
    On Error Goto 0

    strOutput = objExec.StdOut.ReadAll()
    arrLines = Split(strOutput, vbCRLF)

    ReDim arrUniqueProcesses(0)
    count = 0

    For i = 0 To UBound(arrLines)
        Dim line, parts, j
        line = Trim(arrLines(i))

        If Len(line) > 0 And InStr(1, line, ".exe", 1) > 0 Then
            parts = Split(line, " ")
            If UBound(parts) >= 0 Then
                processName = LCase(parts(0))
                found = False

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

    If count > 0 Then
        msg = "The following applications have loaded McBopomofoTIP*.dll:" & vbCRLF & vbCRLF

        For i = 0 To count - 1
            msg = msg & arrUniqueProcesses(i) & vbCRLF
        Next

        msg = msg & vbCRLF & "These applications must be closed before installation can continue." & vbCRLF & vbCRLF
        msg = msg & "Click OK to close them automatically, or Cancel to stop the installation."

        msgResult = MsgBox(msg, vbExclamation + vbOKCancel, "McBopomofo Installer - Close Applications")

        If msgResult = vbCancel Then
            Err.Raise vbObjectError + 1, "CheckAndCloseAppsWithTIP", "User cancelled the installation."
        End If

        For i = 0 To count - 1
            objShell.Run "taskkill.exe /f /im " & arrUniqueProcesses(i), 0, False
        Next

        objShell.Run "taskkill.exe /f /im ctfmon.exe", 0, False
        objShell.Run "cmd.exe /c ping 127.0.0.1 -n 2 > nul", 0, True
        objShell.Run "ctfmon.exe", 0, False
    End If
End Sub

Main
