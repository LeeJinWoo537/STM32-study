' 더블클릭하면 브라우저에서 발표 자료를 엽니다 (한글 경로/파일명 문제 방지)
Set fso = CreateObject("Scripting.FileSystemObject")
Set shell = CreateObject("WScript.Shell")
folder = fso.GetParentFolderName(WScript.ScriptFullName)
htmlPath = fso.BuildPath(folder, "presentation.html")
If fso.FileExists(htmlPath) Then
  shell.Run """" & htmlPath & """", 1, False
Else
  htmlPath = fso.BuildPath(folder, "ICM20948_학습_발표.html")
  If fso.FileExists(htmlPath) Then
    shell.Run """" & htmlPath & """", 1, False
  Else
    MsgBox "발표 파일을 찾을 수 없습니다." & vbCrLf & "presentation.html 또는 ICM20948_학습_발표.html 을 이 폴더에 두세요.", 48, "ICM20948 발표"
  End If
End If
