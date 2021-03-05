@echo off

REM 
REM All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
REM its licensors.
REM
REM For complete copyright and license terms please see the LICENSE at the root of this
REM distribution (the "License"). All use of this software is governed by the License,
REM or, if provided, by the license below or the license accompanying this file. Do not
REM remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
REM WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
REM

:: Launches Wing IDE and the DccScriptingInterface Project Files

:: Set up window
TITLE Lumberyard DCC Scripting Interface GEM WingIDE 7x
:: Use obvious color to prevent confusion (Grey with Yellow Text)
COLOR 8E

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
echo     ABS_PATH = %ABS_PATH%

:: WingIDE version Major
set WING_VERSION_MAJOR=7
echo     WING_VERSION_MAJOR = %WING_VERSION_MAJOR%

:: WingIDE version Major
set WING_VERSION_MINOR=1
echo     WING_VERSION_MINOR = %WING_VERSION_MINOR%

:: note the changed path from IDE to Pro
set WINGHOME=%PROGRAMFILES(X86)%\Wing Pro %WING_VERSION_MAJOR%.%WING_VERSION_MINOR%
echo     WINGHOME = %WINGHOME%

:: Constant Vars (Global)
:: global debug (propogates)
IF "%DCCSI_GDEBUG%"=="" (set DCCSI_GDEBUG=True)
echo     DCCSI_GDEBUG = %DCCSI_GDEBUG%
:: initiates debugger connection
IF "%DCCSI_DEV_MODE%"=="" (set DCCSI_DEV_MODE=True)
echo     DCCSI_DEV_MODE = %DCCSI_DEV_MODE%
:: sets debugger, options: WING, PYCHARM
IF "%DCCSI_GDEBUGGER%"=="" (set DCCSI_GDEBUGGER=WING)
echo     DCCSI_GDEBUGGER = %DCCSI_GDEBUGGER%
:: Default level logger will handle
:: CRITICAL:50
:: ERROR:40
:: WARNING:30
:: INFO:20
:: DEBUG:10
:: NOTSET:0
IF "%DCCSI_LOGLEVEL%"=="" (set DCCSI_LOGLEVEL=10)
echo     DCCSI_LOGLEVEL = %DCCSI_LOGLEVEL%

CALL %~dp0\Env.bat

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