@echo off
:: Launches Wing IDE and the DccScriptingInterface Project Files

REM 
REM Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
REM 
REM SPDX-License-Identifier: Apache-2.0 OR MIT
REM
REM

echo.
echo _____________________________________________________________________
echo.
echo ~    Setting up LY DCCsi WingIDE Dev Env...
echo _____________________________________________________________________
echo.

:: Store current dir
%~d0
cd %~dp0
PUSHD %~dp0

:: Keep changes local
SETLOCAL enableDelayedExpansion

SET ABS_PATH=%~dp0
echo     Current Dir, %ABS_PATH%

:: WingIDE version Major
SET WING_VERSION_MAJOR=7
echo     WING_VERSION_MAJOR = %WING_VERSION_MAJOR%

:: WingIDE version Major
SET WING_VERSION_MINOR=1
echo     WING_VERSION_MINOR = %WING_VERSION_MINOR%

:: note the changed path from IDE to Pro
set WINGHOME=%PROGRAMFILES(X86)%\Wing Pro %WING_VERSION_MAJOR%.%WING_VERSION_MINOR%
echo     WINGHOME = %WINGHOME%

CALL %~dp0\Project_Env.bat

echo.
echo _____________________________________________________________________
echo.
echo ~    WingIDE Version %WING_VERSION_MAJOR%.%WING_VERSION_MINOR%
echo _____________________________________________________________________
echo.

SET WING_PROJ=%DCCSIG_PATH%\Solutions\.wing\DCCsi_%WING_VERSION_MAJOR%x.wpr
echo     WING_PROJ = %WING_PROJ%

echo.
echo _____________________________________________________________________
echo.
echo ~ Launching %LY_PROJECT% project in WingIDE %WING_VERSION_MAJOR%.%WING_VERSION_MINOR% ...
echo _____________________________________________________________________
echo.


IF EXIST "%WINGHOME%\bin\wing.exe" (
   start "" "%WINGHOME%\bin\wing.exe" "%WING_PROJ%"
) ELSE (
   Where wing.exe 2> NUL
   IF ERRORLEVEL 1 (
      echo wing.exe could not be found
      pause
   ) ELSE (
      start "" wing.exe "%WING_PROJ%"
   )
)

ENDLOCAL

:: Return to starting directory
POPD

:END_OF_FILE
