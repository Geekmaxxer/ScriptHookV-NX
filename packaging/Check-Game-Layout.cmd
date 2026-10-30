@echo off
setlocal
if "%~1"=="" goto prompt
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Check-Game-Layout.ps1" %*
exit /b %errorlevel%
:prompt
echo GTA game layout check. This action makes no changes.
echo Enter the SD root, for example D:\
set /p "SHV_SD_ROOT=SD root: "
if not defined SHV_SD_ROOT exit /b 1
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Check-Game-Layout.ps1" -SdRoot "%SHV_SD_ROOT%"
set "SHV_CHECK_RESULT=%errorlevel%"
echo.
echo Read GAME-STARTUP.md for recovery and DLC space-saving guidance.
pause
exit /b %SHV_CHECK_RESULT%
