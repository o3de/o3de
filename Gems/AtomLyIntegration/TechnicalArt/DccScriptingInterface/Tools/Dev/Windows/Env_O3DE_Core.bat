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

:: if the user has set up a custom env call it
IF EXIST "%~dp0Env_Dev.bat" CALL %~dp0Env_Dev.bat

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

IF "%DCCSI_TESTS%"=="" (set DCCSI_TESTS=false)
echo     DCCSI_TESTS = %DCCSI_TESTS%

:: This maps up to the \Dev folder
IF "%O3DE_REL_PATH%"=="" (set "O3DE_REL_PATH=..\..\..\..")
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
IF "%PATH_O3DE_PROJECT%"=="" (set "PATH_O3DE_PROJECT=%CD%")
echo     PATH_O3DE_PROJECT = %PATH_O3DE_PROJECT%

IF "%ABS_PATH%"=="" (set ABS_PATH=%CD%)
echo     ABS_PATH = %ABS_PATH%

:: Save current directory and change to target directory
pushd %ABS_PATH%

:: Change to root Lumberyard dev dir
CD /d %PATH_O3DE_PROJECT%\%O3DE_REL_PATH%
IF "%O3DE_DEV%"=="" (set "O3DE_DEV=%CD%")
echo     O3DE_DEV = %O3DE_DEV%
:: Restore original directory
popd

:: We need to know where the engine 3rdParty folder is to add access for PySide2, etc
:: Adding 3rdParty location, default is something like c:\users\< user>\.o3de\3rdparty
IF "%PATH_O3DE_3RDPARTY%"=="" (set "PATH_O3DE_3RDPARTY=%userprofile%\.o3de\3rdparty")
echo     PATH_O3DE_3RDPARTY = %PATH_O3DE_3RDPARTY%

:: O3DE Technical Art Gems Location
set "PATH_O3DE_TECHART_GEMS=%O3DE_DEV%\Gems\AtomLyIntegration\TechnicalArt"
echo     PATH_O3DE_TECHART_GEMS = %PATH_O3DE_TECHART_GEMS%

:: dcc scripting interface gem path
:: currently know relative path to this gem
set "PATH_DCCSIG=%O3DE_DEV%\Gems\AtomLyIntegration\TechnicalArt\DccScriptingInterface"
echo     PATH_DCCSIG = %PATH_DCCSIG%

:: Change to DCCsi root dir
CD /D %PATH_DCCSIG%

:: dccsi tools path
set "PATH_DCCSI_TOOLS=%PATH_DCCSIG%\Tools"
echo     PATH_DCCSI_TOOLS = %PATH_DCCSI_TOOLS%

:: dccsi tools path
set "PATH_DCCSI_TOOLS_DCC=%PATH_DCCSIG%\Tools\DCC"
echo     PATH_DCCSI_TOOLS_DCC = %PATH_DCCSI_TOOLS_DCC%

:: temp log location specific to this gem
set "DCCSI_LOG_PATH=%PATH_O3DE_PROJECT%\.temp\logs"
echo     DCCSI_LOG_PATH = %DCCSI_LOG_PATH%

IF "%O3DE_BUILD_FOLDER%"=="" (set "O3DE_BUILD_FOLDER=bin\Windows\profile\Default")
echo     O3DE_BUILD_FOLDER = %O3DE_BUILD_FOLDER%

:: for reference the nightly engine sdk bin path looks like:
:: C:\O3DE\0.0.0.0\bin\Windows\profile\Default
IF "%PATH_O3DE_BIN%"=="" (set "PATH_O3DE_BIN=%O3DE_DEV%\%O3DE_BUILD_FOLDER%")
echo     PATH_O3DE_BIN = %PATH_O3DE_BIN%

::ENDLOCAL

:: Set flag so we don't initialize dccsi environment twice
SET DCCSI_ENV_INIT=1
GOTO END_OF_FILE

:: Return to starting directory
POPD

:END_OF_FILE
