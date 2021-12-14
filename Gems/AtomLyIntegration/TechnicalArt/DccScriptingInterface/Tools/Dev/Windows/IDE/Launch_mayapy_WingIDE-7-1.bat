@echo off

REM 
REM Copyright (c) Contributors to the Open 3D Engine Project.
REM For complete copyright and license terms please see the LICENSE at the root of this distribution.
REM
REM SPDX-License-Identifier: Apache-2.0 OR MIT
REM
REM

:: Launches Wing IDE and the DccScriptingInterface Project Files

:: Set up window
TITLE O3DE DCCsi Launch WingIDE 7x
:: Use obvious color to prevent confusion (Grey with Yellow Text)
COLOR 8E

:: Store current dir
%~d0
cd %~dp0
PUSHD %~dp0

:: Constant Vars (Global)
:: global debug (propogates)
IF "%DCCSI_GDEBUG%"=="" (set DCCSI_GDEBUG=False)
echo     DCCSI_GDEBUG = %DCCSI_GDEBUG%
:: initiates debugger connection
IF "%DCCSI_DEV_MODE%"=="" (set DCCSI_DEV_MODE=False)
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
IF "%DCCSI_LOGLEVEL%"=="" (set DCCSI_LOGLEVEL=20)
echo     DCCSI_LOGLEVEL = %DCCSI_LOGLEVEL%

:: if the user has set up a custom env call it
IF EXIST "%~dp0..\Env_Dev.bat" CALL %~dp0..\Env_Dev.bat

:: Initialize env
CALL %~dp0\..\Env_Core.bat
CALL %~dp0\..\Env_Python.bat
CALL %~dp0\..\Env_WingIDE.bat
CALL %~dp0\..\Env_Maya.bat

echo.
echo _____________________________________________________________________
echo.
echo ~ WingIDE Version %DCCSI_WING_VERSION_MAJOR%.%DCCSI_WING_VERSION_MINOR%
echo ~ Launching O3DE %O3DE_PROJECT% project in WingIDE %DCCSI_WING_VERSION_MAJOR%.%DCCSI_WING_VERSION_MINOR% ...
echo ~ MayaPy.exe (default python interpreter)
echo _____________________________________________________________________
echo.

echo     O3DE_DEV = %O3DE_DEV%

:: ide and debugger plug
set DCCSI_PY_DEFAULT=%DCCSI_PY_MAYA%
echo     DCCSI_PY_DEFAULT = %DCCSI_PY_DEFAULT%

echo.

:: Change to root dir
CD /D %PATH_O3DE_PROJECT%

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

:: Return to starting directory
POPD

:END_OF_FILE
