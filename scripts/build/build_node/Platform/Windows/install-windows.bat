@echo off
@SETLOCAL enableDelayedExpansion

rem Constants for the install script
rem DP0 includes a slash at the end, so we capture it in a SET to append data after it...
set BATCH_FILE_DIR=%~dp0

rem detect whether we are double clicked or running from a command line:
@echo %SCRIPT:~0,1% | findstr /l %DQUOTE% > NUL
if %ERRORLEVEL% EQU 0 set WAS_DOUBLE_CLICKED=1

PowerShell -NoProfile -Command "& {Start-Process PowerShell -ArgumentList '-NoProfile -ExecutionPolicy Bypass -File ""./install-windows-buildtools.ps1""' -Verb RunAs}" 
PowerShell -NoProfile -Command "& {Start-Process PowerShell -ArgumentList '-NoProfile -ExecutionPolicy Bypass -File ""./install-windows-python3.ps1""' -Verb RunAs}" 
PowerShell -NoProfile -Command "& {Start-Process PowerShell -ArgumentList '-NoProfile -ExecutionPolicy Bypass -File ""./install-windows-vsbuildtools.ps1""' -Verb RunAs}" 
PowerShell -NoProfile -Command "& {Start-Process PowerShell -ArgumentList '-NoProfile -ExecutionPolicy Bypass -File ""./install-windows-android.ps1""' -Verb RunAs}" 
