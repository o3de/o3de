@echo off
REM 
REM Copyright (c) Contributors to the Open 3D Engine Project.
REM For complete copyright and license terms please see the LICENSE at the root of this distribution.
REM
REM SPDX-License-Identifier: Apache-2.0 OR MIT
REM
REM

:: Sets up extended DCCsi environment for Blender

:: Set up window
TITLE O3DE DCCsi Blender Environment
:: Use obvious color to prevent confusion (Grey with Yellow Text)
COLOR 8E

:: Skip initialization if already completed
IF "%DCCSI_ENV_BLENDER_INIT%"=="1" GOTO :END_OF_FILE

:: Store current dir
%~d0
cd %~dp0
PUSHD %~dp0

:: Consider experimental, only tested with Blender 3.0 during initial development
:: Blender 3.0 :: Python 3.9.7 (default, Oct 11 2021, 19:31:28) [MSC v.1916 64 bit (AMD64)] on win32
IF "%DCCSI_PY_VERSION_MAJOR%"=="" (set DCCSI_PY_VERSION_MAJOR=3)
IF "%DCCSI_PY_VERSION_MINOR%"=="" (set DCCSI_PY_VERSION_MINOR=9)
IF "%DCCSI_PY_VERSION_RELEASE%"=="" (set DCCSI_PY_VERSION_RELEASE=7)

:: Default Blender Version
IF "%DCCSI_BLENDER_VERSION%"=="" (set DCCSI_BLENDER_VERSION=3.1)

:: Initialize env
CALL %~dp0\Env_O3DE_Core.bat
CALL %~dp0\Env_O3DE_Python.bat

:: This can now only be added late, in the launcher
:: it conflicts with other Qt apps like Wing Pro 8+
::CALL %~dp0\Env_O3DE_Qt.bat
:: this could interfere with standalone python apps/tools/utils that use O3DE Qt
:: and trying to run them from the IDE
:: We may have to find a work around in the next iteration?

echo.
echo _____________________________________________________________________
echo.
echo ~    O3DE DCCsi Blender Environment ...
echo _____________________________________________________________________
echo.

echo     DCCSI_PY_VERSION_MAJOR = %DCCSI_PY_VERSION_MAJOR%
echo     DCCSI_PY_VERSION_MINOR = %DCCSI_PY_VERSION_MINOR%
echo     DCCSI_PY_VERSION_RELEASE = %DCCSI_PY_VERSION_RELEASE%
echo     DCCSI_BLENDER_VERSION = %DCCSI_BLENDER_VERSION%

:::: Set Blender native access to this project
IF "%DCCSI_BLENDER_PROJECT%"=="" (set DCCSI_BLENDER_PROJECT=%PATH_O3DE_PROJECT%)
echo     DCCSI_BLENDER_PROJECT = %DCCSI_BLENDER_PROJECT%

:: set dccsi blender tools path
set "DCCSI_TOOLS_BLENDER_PATH=%PATH_DCCSI_TOOLS%\DCC\Blender"
echo     DCCSI_TOOLS_BLENDER_PATH = %DCCSI_TOOLS_BLENDER_PATH%

IF "%DCCSI_BLENDER_LOCATION%"=="" (set "DCCSI_BLENDER_LOCATION=%ProgramFiles%\Blender Foundation\Blender %DCCSI_BLENDER_VERSION%")
echo     DCCSI_BLENDER_LOCATION = %DCCSI_BLENDER_LOCATION%

IF "%DCCSI_BLENDER_EXE%"=="" (set "DCCSI_BLENDER_EXE=%DCCSI_BLENDER_LOCATION%\blender.exe")
echo     DCCSI_BLENDER_EXE = %DCCSI_BLENDER_EXE%

IF "%DCCSI_BLENDER_LAUNCHER%"=="" (set "DCCSI_BLENDER_LAUNCHER=%DCCSI_BLENDER_LOCATION%\blender-launcher.exe")
echo     DCCSI_BLENDER_LAUNCHER = %DCCSI_BLENDER_LAUNCHER%

IF "%DCCSI_BLENDER_PYTHON%"=="" (set "DCCSI_BLENDER_PYTHON=%DCCSI_BLENDER_LOCATION%\%DCCSI_BLENDER_VERSION%\python")
echo     DCCSI_BLENDER_PYTHON = %DCCSI_BLENDER_PYTHON%

IF "%DCCSI_BLENDER_PY_BIN%"=="" (set "DCCSI_BLENDER_PY_BIN=%DCCSI_BLENDER_PYTHON%\bin")
echo     DCCSI_BLENDER_PY_BIN = %DCCSI_BLENDER_PY_BIN%

IF "%DCCSI_BLENDER_PY_EXE%"=="" (set "DCCSI_BLENDER_PY_EXE=%DCCSI_BLENDER_PY_BIN%\python.exe")
echo     DCCSI_BLENDER_PY_EXE = %DCCSI_BLENDER_PY_EXE%

set "DCCSI_PY_BLENDER=%DCCSI_BLENDER_PY_EXE%"
echo     DCCSI_PY_BLENDER = %DCCSI_PY_BLENDER%

IF "%DCCSI_BLENDER_SET_CALLBACKS%"=="" (set DCCSI_BLENDER_SET_CALLBACKS=false)
echo     DCCSI_BLENDER_SET_CALLBACKS = %DCCSI_BLENDER_SET_CALLBACKS%

IF "%DCCSI_BLENDER_SCRIPTS%"=="" (set "DCCSI_BLENDER_SCRIPTS=%DCCSI_BLENDER_LOCATION%\Scripts")
echo     DCCSI_BLENDER_SCRIPTS = %DCCSI_BLENDER_SCRIPTS%

IF "%DCCSI_BLENDER_ADDONS%"=="" (set "DCCSI_BLENDER_ADDONS=%DCCSI_BLENDER_SCRIPTS%\AddOns")
echo     DCCSI_BLENDER_ADDONS = %DCCSI_BLENDER_ADDONS%

:: Only launchers should actually set the PATH and PYTHONPATH !!!
:: add to the PATH
::SET PATH=%DCCSI_BLENDER_LOCATION%;%DCCSI_BLENDER_PY_BIN%;%PATH%

:: add all python related paths to PYTHONPATH for package imports
::set PYTHONPATH=%DCCSI_BLENDER_SCRIPTS%;%DCCSI_BLENDER_SCRIPTS%;%PYTHONPATH%
::echo     PYTHONPATH = %PYTHONPATH%

::ENDLOCAL

:: Set flag so we don't initialize dccsi environment twice
SET DCCSI_ENV_BLENDER_INIT=1
GOTO END_OF_FILE

:: Return to starting directory
POPD

:END_OF_FILE
