@echo off
REM 
REM Copyright (c) Contributors to the Open 3D Engine Project.
REM For complete copyright and license terms please see the LICENSE at the root of this distribution.
REM
REM SPDX-License-Identifier: Apache-2.0 OR MIT
REM
REM

:: Sets up extended DCCsi environment for Maya

:: Set up window
TITLE O3DE DCCsi Autodesk Maya Environment
:: Use obvious color to prevent confusion (Grey with Yellow Text)
COLOR 8E

:: Skip initialization if already completed
IF "%DCCSI_ENV_MAYA_INIT%"=="1" GOTO :END_OF_FILE

:: Store current dir
%~d0
cd %~dp0
PUSHD %~dp0

IF "%DCCSI_PY_VERSION_MAJOR%"=="" (set DCCSI_PY_VERSION_MAJOR=2)
IF "%DCCSI_PY_VERSION_MINOR%"=="" (set DCCSI_PY_VERSION_MINOR=7)
IF "%DCCSI_PY_VERSION_RELEASE%"=="" (set DCCSI_PY_VERSION_RELEASE=11)

:: Default Maya Version
IF "%DCCSI_MAYA_VERSION%"=="" (set DCCSI_MAYA_VERSION=2020)

:: Initialize env
CALL %~dp0\Env_Core.bat
CALL %~dp0\Env_Python.bat

echo.
echo _____________________________________________________________________
echo.
echo ~    O3DE DCCsi Autodesk Maya Environment ...
echo _____________________________________________________________________
echo.

echo     DCCSI_PY_VERSION_MAJOR = %DCCSI_PY_VERSION_MAJOR%
echo     DCCSI_PY_VERSION_MINOR = %DCCSI_PY_VERSION_MINOR%
echo     DCCSI_PY_VERSION_RELEASE = %DCCSI_PY_VERSION_RELEASE%
echo     DCCSI_MAYA_VERSION = %DCCSI_MAYA_VERSION%

:::: Set Maya native project acess to this project
IF "%MAYA_PROJECT%"=="" (set MAYA_PROJECT=%O3DE_PROJECT%)
echo     MAYA_PROJECT = %MAYA_PROJECT%

:: maya sdk path
set DCCSI_TOOLS_MAYA_PATH=%DCCSI_TOOLS_PATH%\DCC\Maya
echo     DCCSI_TOOLS_MAYA_PATH = %DCCSI_TOOLS_MAYA_PATH%

set MAYA_MODULE_PATH=%DCCSI_TOOLS_MAYA_PATH%;%MAYA_MODULE_PATH%
echo     MAYA_MODULE_PATH = %MAYA_MODULE_PATH%

:: Maya File Paths, etc
:: https://knowledge.autodesk.com/support/maya/learn-explore/caas/CloudHelp/cloudhelp/2015/ENU/Maya/files/Environment-Variables-File-path-variables-htm.html
:::: Set Maya native project acess to this project
IF "%MAYA_LOCATION%"=="" (set MAYA_LOCATION=%ProgramFiles%\Autodesk\Maya%DCCSI_MAYA_VERSION%)
echo     MAYA_LOCATION = %MAYA_LOCATION%

IF "%MAYA_BIN_PATH%"=="" (set MAYA_BIN_PATH=%MAYA_LOCATION%\bin)
echo     MAYA_BIN_PATH = %MAYA_BIN_PATH%

:: these improve the boot up time (Ha ... not really much)
IF "%MAYA_DISABLE_CIP%"=="" (set MAYA_DISABLE_CIP=1)
echo     MAYA_DISABLE_CIP = %MAYA_DISABLE_CIP%

IF "%MAYA_DISABLE_CER%"=="" (set MAYA_DISABLE_CER=1)
echo     MAYA_DISABLE_CER = %MAYA_DISABLE_CER%

IF "%MAYA_DISABLE_CLIC_IPM%"=="" (set MAYA_DISABLE_CLIC_IPM=1)
echo     MAYA_DISABLE_CLIC_IPM = %MAYA_DISABLE_CLIC_IPM%

IF "%DCCSI_MAYA_SET_CALLBACKS%"=="" (set DCCSI_MAYA_SET_CALLBACKS=false)
echo     DCCSI_MAYA_SET_CALLBACKS = %DCCSI_MAYA_SET_CALLBACKS%

:: setting this to 1 should further improve boot time (I think)
::IF "%MAYA_NO_CONSOLE_WINDOW%"=="" (set MAYA_NO_CONSOLE_WINDOW=0)
::echo     MAYA_NO_CONSOLE_WINDOW = %MAYA_NO_CONSOLE_WINDOW%
:: But I like the console window for development and debugging early boot

:: shared location for 64bit DCCSI_PY_MAYA python location (2.7)
set DCCSI_PY_MAYA=%MAYA_BIN_PATH%\mayapy.exe
echo     DCCSI_PY_MAYA = %DCCSI_PY_MAYA%

:: add to the PATH
SET PATH=%MAYA_BIN_PATH%;%PATH%

:: Local DCCsi Maya plugins access (ours)
set DCCSI_MAYA_PLUG_IN_PATH=%DCCSI_TOOLS_MAYA_PATH%\plugins
:: also attached to maya's built-it env var
set MAYA_PLUG_IN_PATH=%DCCSI_MAYA_PLUG_IN_PATH%;MAYA_PLUG_IN_PATH
echo     DCCSI_MAYA_PLUG_IN_PATH = %DCCSI_MAYA_PLUG_IN_PATH%

:: Local DCCsi Maya shelves (ours)
set DCCSI_MAYA_SHELF_PATH=%DCCSI_TOOLS_MAYA_PATH%\Prefs\Shelves
set MAYA_SHELF_PATH=%DCCSI_MAYA_SHELF_PATH%
echo     DCCSI_MAYA_SHELF_PATH = %DCCSI_MAYA_SHELF_PATH%

:: Local DCCsi Maya icons path (ours)
set DCCSI_MAYA_XBMLANGPATH=%DCCSI_TOOLS_MAYA_PATH%\Prefs\icons
:: also attached to maya's built-it env var
set XBMLANGPATH=%DCCSI_MAYA_XBMLANGPATH%;%XBMLANGPATH%
echo     DCCSI_MAYA_XBMLANGPATH = %DCCSI_MAYA_XBMLANGPATH%

:: Local DCCsi Maya Mel scripts (ours)
set DCCSI_MAYA_SCRIPT_MEL_PATH=%DCCSI_TOOLS_MAYA_PATH%\Scripts\Mel
:: also attached to maya's built-it env var
set MAYA_SCRIPT_PATH=%DCCSI_MAYA_SCRIPT_MEL_PATH%;%MAYA_SCRIPT_PATH%
echo     DCCSI_MAYA_SCRIPT_MEL_PATH = %DCCSI_MAYA_SCRIPT_MEL_PATH%

:: Local DCCsi Maya Python scripts (ours)
set DCCSI_MAYA_SCRIPT_PY_PATH=%DCCSI_TOOLS_MAYA_PATH%\Scripts\Python
:: also attached to maya's built-it env var
set MAYA_SCRIPT_PATH=%DCCSI_MAYA_SCRIPT_PY_PATH%;%MAYA_SCRIPT_PATH%
echo     DCCSI_MAYA_SCRIPT_PY_PATH = %DCCSI_MAYA_SCRIPT_PY_PATH%

:: DCCsi Maya boostrap, userSetup.py access (ours)
set DCCSI_MAYA_SCRIPT_PATH=%DCCSI_TOOLS_MAYA_PATH%\Scripts
:: also attached to maya's built-it env var
set MAYA_SCRIPT_PATH=%DCCSI_MAYA_SCRIPT_PATH%;%MAYA_SCRIPT_PATH%
echo     DCCSI_MAYA_SCRIPT_PATH = %DCCSI_MAYA_SCRIPT_PATH%

echo     MAYA_SCRIPT_PATH = %MAYA_SCRIPT_PATH%

:: add all python related paths to PYTHONPATH for package imports
set PYTHONPATH=%DCCSI_MAYA_SCRIPT_PATH%;%DCCSI_MAYA_SCRIPT_PY_PATH%;%PYTHONPATH%
echo     PYTHONPATH = %PYTHONPATH%

:: DX11 Viewport
Set MAYA_VP2_DEVICE_OVERRIDE=VirtualDeviceDx11
echo     MAYA_VP2_DEVICE_OVERRIDE = %MAYA_VP2_DEVICE_OVERRIDE%

::ENDLOCAL

:: Set flag so we don't initialize dccsi environment twice
SET DCCSI_ENV_MAYA_INIT=1
GOTO END_OF_FILE

:: Return to starting directory
POPD

:END_OF_FILE
