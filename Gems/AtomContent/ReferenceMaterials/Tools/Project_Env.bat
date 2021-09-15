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
TITLE O3DE Asset Gem
:: Use obvious color to prevent confusion (Grey with Yellow Text)
COLOR 8E

:: Skip initialization if already completed
IF "%O3DE_PROJ_ENV_INIT%"=="1" GOTO :END_OF_FILE

:: Store current dir
%~d0
cd %~dp0
PUSHD %~dp0

:: Put you project env vars and overrides in this file

:: chanhe the relative path up to dev
set ABS_PATH=%~dp0

:: project name as a str tag
IF "%LY_PROJECT_NAME%"=="" (
    for %%I in ("%~dp0.") do for %%J in ("%%~dpI.") do set LY_PROJECT_NAME=%%~nxJ
    )

echo.
echo _____________________________________________________________________
echo.
echo ~    Setting up O3DE %LY_PROJECT_NAME% Environment ...
echo _____________________________________________________________________
echo.
echo     LY_PROJECT_NAME = %LY_PROJECT_NAME%

:: if the user has set up a custom env call it
:: this should allow the user to locally
:: set env hooks like LY_DEV or LY_PROJECT
IF EXIST "%~dp0User_Env.bat" CALL %~dp0User_Env.bat
echo     LY_DEV = %LY_DEV%

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

:: Override the default maya version
IF "%DCCSI_MAYA_VERSION%"=="" (set DCCSI_MAYA_VERSION=2020)
echo     DCCSI_MAYA_VERSION = %DCCSI_MAYA_VERSION%

:: LY_PROJECT is ideally treated as a full path in the env launchers
:: do to changes in o3de, external engine/project/gem folder structures, etc.
IF "%LY_PROJECT%"=="" (
    for %%i in ("%~dp0..") do set "LY_PROJECT=%%~fi"
    )
echo     LY_PROJECT = %LY_PROJECT%

:: this is here for archaic reasons, WILL DEPRECATE
IF "%LY_PROJECT_PATH%"=="" (set LY_PROJECT_PATH=%LY_PROJECT%)
echo     LY_PROJECT_PATH = %LY_PROJECT_PATH%

:: Change to root Lumberyard dev dir
:: You must set this in a User_Env.bat to match youe engine repo location!
IF "%LY_DEV%"=="" (set LY_DEV=C:\Depot\o3de-engine)
echo     LY_DEV = %LY_DEV%

CALL %LY_DEV%\Gems\AtomLyIntegration\TechnicalArt\DccScriptingInterface\Launchers\Windows\Env_Maya.bat

:: Restore original directory
popd

:: Change to root dir
CD /D %ABS_PATH%

::ENDLOCAL

:: Set flag so we don't initialize dccsi environment twice
SET O3DE_PROJ_ENV_INIT=1
GOTO END_OF_FILE

:: Return to starting directory
POPD

:END_OF_FILE
