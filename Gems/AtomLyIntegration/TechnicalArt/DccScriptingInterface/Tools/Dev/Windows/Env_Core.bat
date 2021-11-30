@echo off
REM 
REM Copyright (c) Contributors to the Open 3D Engine Project.
REM For complete copyright and license terms please see the LICENSE at the root of this distribution.
REM
REM SPDX-License-Identifier: Apache-2.0 OR MIT
REM
REM

:: Sets up environment for Lumberyard DCC tools and code access

:: Set up window
TITLE O3DE DCC Scripting Interface Core
:: Use obvious color to prevent confusion (Grey with Yellow Text)
COLOR 8E

:: Skip initialization if already completed
IF "%DCCSI_ENV_INIT%"=="1" GOTO :END_OF_FILE

:: Store current dir
%~d0
cd %~dp0
PUSHD %~dp0

::SETLOCAL ENABLEDELAYEDEXPANSION

echo.
echo _____________________________________________________________________
echo.
echo ~    O3DE DCC Scripting Interface Environment ...
echo _____________________________________________________________________
echo.

IF "%DCCSI_DEV_ENV%"=="" (set DCCSI_DEV_ENV=%~dp0)
echo     DCCSI_DEV_ENV = %DCCSI_DEV_ENV%

:: add to the PATH
SET PATH=%DCCSI_DEV_ENV%;%PATH%

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
IF "%O3DE_REL_PATH%"=="" (set O3DE_REL_PATH=..\..\..\..)
echo     O3DE_REL_PATH = %O3DE_REL_PATH%

:: You can define the project name
IF "%O3DE_PROJECT%"=="" (
    for %%a in (%CD%..\..\..\..) do set O3DE_PROJECT=%%~na
        )
echo     O3DE_PROJECT = %O3DE_PROJECT%

:: set up the default project path (dccsi)
:: if not set we also use the DCCsi path as stand-in
CD /D ..\..\..\
:: To Do: remove one of these 
IF "%O3DE_PROJECT_PATH%"=="" (set O3DE_PROJECT_PATH=%CD%)
echo     O3DE_PROJECT_PATH = %O3DE_PROJECT_PATH%

IF "%ABS_PATH%"=="" (set ABS_PATH=%CD%)
echo     ABS_PATH = %ABS_PATH%

:: Save current directory and change to target directory
pushd %ABS_PATH%

:: Change to root Lumberyard dev dir
CD /d %O3DE_PROJECT_PATH%\%O3DE_REL_PATH%
IF "%O3DE_DEV%"=="" (set O3DE_DEV=%CD%)
echo     O3DE_DEV = %O3DE_DEV%
:: Restore original directory
popd

:: dcc scripting interface gem path
:: currently know relative path to this gem
set DCCSIG_PATH=%O3DE_DEV%\Gems\AtomLyIntegration\TechnicalArt\DccScriptingInterface
echo     DCCSIG_PATH = %DCCSIG_PATH%

:: Change to DCCsi root dir
CD /D %DCCSIG_PATH%

:: per-dcc sdk path
set DCCSI_TOOLS_PATH=%DCCSIG_PATH%\Tools
echo     DCCSI_TOOLS_PATH = %DCCSI_TOOLS_PATH%

:: temp log location specific to this gem
set DCCSI_LOG_PATH=%O3DE_PROJECT_PATH%\.temp\logs
echo     DCCSI_LOG_PATH = %DCCSI_LOG_PATH%

:: O3DE build path
IF "%O3DE_BUILD_FOLDER%"=="" (set O3DE_BUILD_FOLDER=build)
echo     O3DE_BUILD_FOLDER = %O3DE_BUILD_FOLDER%

IF "%O3DE_BUILD_PATH%"=="" (set O3DE_BUILD_PATH=%O3DE_DEV%\%O3DE_BUILD_FOLDER%)
echo     O3DE_BUILD_PATH = %O3DE_BUILD_PATH%

IF "%O3DE_BIN_PATH%"=="" (set O3DE_BIN_PATH=%O3DE_BUILD_PATH%\bin\profile)
echo     O3DE_BIN_PATH = %O3DE_BIN_PATH%

:: add to the PATH
SET PATH=%O3DE_BIN_PATH%;%DCCSIG_PATH%;%PATH%

::ENDLOCAL

:: Set flag so we don't initialize dccsi environment twice
SET DCCSI_ENV_INIT=1
GOTO END_OF_FILE

:: Return to starting directory
POPD

:END_OF_FILE
