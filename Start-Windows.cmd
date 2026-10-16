@echo off
setlocal
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\Start-Windows.ps1"
if errorlevel 1 (
  echo.
  echo Setup stopped. Read the message above; your ROMs were not uploaded.
)
pause
