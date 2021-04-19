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

:: Set up window
TITLE O3DE DCCsi GEM PyCharm
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
IF "%DCCSI_LOGLEVEL%"=="" (set DCCSI_LOGLEVEL=10)
echo     DCCSI_LOGLEVEL = %DCCSI_LOGLEVEL%

:: WingIDE version Major
IF "%DCCSI_WING_VERSION_MAJOR%"=="" (set DCCSI_WING_VERSION_MAJOR=7)
echo DCCSI_WING_VERSION_MAJOR = %DCCSI_WING_VERSION_MAJOR%
:: WingIDE version Major
IF "%DCCSI_WING_VERSION_MINOR%"=="" (set DCCSI_WING_VERSION_MINOR=2)
echo DCCSI_WING_VERSION_MINOR = %DCCSI_WING_VERSION_MINOR%

:: Initialize env
CALL %~dp0\Env_PyCharm.bat

:: if the user has set up a custom env call it
IF EXIST "%~dp0Env_Dev.bat" CALL %~dp0Env_Dev.bat
set WINGHOME=%PROGRAMFILES(X86)%\Wing Pro %DCCSI_WING_VERSION_MAJOR%.%DCCSI_WING_VERSION_MINOR%
echo WINGHOME = %WINGHOME%

echo.
echo _____________________________________________________________________
echo.
echo ~ Launching DCCsi Project in PyCharm %PYCHARM_VERSION_YEAR%.%PYCHARM_VERSION_MAJOR% ...
echo _____________________________________________________________________
echo.

IF EXIST "%PYCHARM_HOME%\bin\pycharm64.exe" (
    start "" "%PYCHARM_HOME%\bin\pycharm64.exe" "%PYCHARM_PROJ%"
) ELSE (
    Where pycharm64.exe 2> NUL
    IF ERRORLEVEL 1 (
        echo pycharm64.exe could not be found
            pause
    ) ELSE (
        start "" pycharm64.exe "%PYCHARM_PROJ%"
    )
)

ENDLOCAL

:: Return to starting directory
POPD

:END_OF_FILE
