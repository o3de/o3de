@echo off

REM 
REM Copyright (c) Contributors to the Open 3D Engine Project
REM 
REM SPDX-License-Identifier: Apache-2.0 OR MIT
REM For complete copyright and license terms please see the LICENSE at the root of this distribution.
REM
REM

:: Sets up environment for O3DE DCC tools and code access

:: Set up window
TITLE O3DE Color Grading Env Core
:: Use obvious color to prevent confusion (Grey with Yellow Text)
COLOR 8E

:: Skip initialization if already completed
IF "%O3DE_ENV_INIT%"=="1" GOTO :END_OF_FILE

:: Store current dir
%~d0
cd %~dp0
PUSHD %~dp0

::SETLOCAL ENABLEDELAYEDEXPANSION

echo.
echo _____________________________________________________________________
echo.
echo ~    O3DE Color Grading Core Env ...
echo _____________________________________________________________________
echo.

IF "%CMD_LAUNCHERS%"=="" (set CMD_LAUNCHERS=%~dp0)
echo     CMD_LAUNCHERS = %CMD_LAUNCHERS%

:: add to the PATH
SET PATH=%CMD_LAUNCHERS%;%PATH%

:: Constant Vars (Global)
:: global debug flag (propogates)
:: The intent here is to set and globally enter a debug mode
IF "%DCCSI_GDEBUG%"=="" (set DCCSI_GDEBUG=false)
echo     DCCSI_GDEBUG = %DCCSI_GDEBUG%
:: initiates earliest debugger connection
:: we support attaching to WingIDE... PyCharm and VScode in the future
IF "%DCCSI_DEV_MODE%"=="" (set DCCSI_DEV_MODE=false)
echo     DCCSI_DEV_MODE = %DCCSI_DEV_MODE%
:: sets debugger, options: WING, PYCHARM
IF "%DCCSI_GDEBUGGER%"=="" (set DCCSI_GDEBUGGER=WING)
echo     DCCSI_GDEBUGGER = %DCCSI_GDEBUGGER%
:: Default level logger will handle
:: Override this to control the setting
:: CRITICAL:50
:: ERROR:40
:: WARNING:30
:: INFO:20
:: DEBUG:10
:: NOTSET:0
IF "%DCCSI_LOGLEVEL%"=="" (set DCCSI_LOGLEVEL=20)
echo     DCCSI_LOGLEVEL = %DCCSI_LOGLEVEL%

:: This maps up to the \Dev folder
IF "%DEV_REL_PATH%"=="" (set DEV_REL_PATH=..\..\..\..\..\..)
echo     DEV_REL_PATH = %DEV_REL_PATH%

:: You can define the project name
IF "%O3DE_PROJECT_NAME%"=="" (
    for %%a in (%CD%..\..) do set O3DE_PROJECT_NAME=%%~na
        )
echo     O3DE_PROJECT_NAME = %O3DE_PROJECT_NAME%

CD /D ..\
IF "%O3DE_PROJECT_PATH%"=="" (set O3DE_PROJECT_PATH=%CD%)
echo     O3DE_PROJECT_PATH = %O3DE_PROJECT_PATH%

IF "%ABS_PATH%"=="" (set ABS_PATH=%CD%)
echo     ABS_PATH = %ABS_PATH%

:: Save current directory and change to target directory
pushd %ABS_PATH%

:: Change to root Lumberyard dev dir
CD /d %DEV_REL_PATH%
IF "%O3DE_DEV%"=="" (set O3DE_DEV=%CD%)
echo     O3DE_DEV = %O3DE_DEV%
:: Restore original directory
popd

:: dcc scripting interface gem path
:: currently know relative path to this gem
set DCCSIG_PATH=%O3DE_DEV%\Gems\AtomLyIntegration\TechnicalArt\DccScriptingInterface
echo     DCCSIG_PATH = %DCCSIG_PATH%

:: per-dcc sdk path
set DCCSI_SDK_PATH=%DCCSIG_PATH%\SDK
echo     DCCSI_SDK_PATH = %DCCSI_SDK_PATH%

:: temp log location specific to this gem
set DCCSI_LOG_PATH=%DCCSIG_PATH%\.temp\logs
echo     DCCSI_LOG_PATH = %DCCSI_LOG_PATH%

:: O3DE build path
IF "%TAG_O3DE_BUILD_PATH%"=="" (set TAG_O3DE_BUILD_PATH=build)
echo     TAG_O3DE_BUILD_PATH = %TAG_O3DE_BUILD_PATH%

IF "%O3DE_BUILD_PATH%"=="" (set O3DE_BUILD_PATH=%O3DE_DEV%\%TAG_O3DE_BUILD_PATH%)
echo     O3DE_BUILD_PATH = %O3DE_BUILD_PATH%

IF "%O3DE_BIN_PATH%"=="" (set O3DE_BIN_PATH=%O3DE_BUILD_PATH%\bin\profile)
echo     O3DE_BIN_PATH = %O3DE_BIN_PATH%

:: add to the PATH
SET PATH=%O3DE_BUILD_PATH%;%DCCSIG_PATH%;%DCCSI_AZPY_PATH%;%PATH%

::ENDLOCAL

:: Set flag so we don't initialize dccsi environment twice
SET O3DE_ENV_INIT=1
GOTO END_OF_FILE

:: Return to starting directory
POPD

:END_OF_FILE
