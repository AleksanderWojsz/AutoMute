@echo off

set "appFolder=%~dp0"
set "startupFolder=%AppData%\Microsoft\Windows\Start Menu\Programs\Startup"
set "appExe=%appFolder%AutoMute.exe"
set "shortcut=%startupFolder%\AutoMute.lnk"
powershell -ExecutionPolicy Bypass -NoProfile -Command "$ws = New-Object -ComObject WScript.Shell; $s = $ws.CreateShortcut('%shortcut%'); $s.TargetPath = '%appExe%'; $s.Save()"