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

:: Maya 2022: 3.7.7 (tags/v3.7.7:d7c567b08f, Mar 10 2020, 10:41:24) [MSC v.1900 64 bit (AMD64)]
IF "%DCCSI_PY_VERSION_MAJOR%"=="" (set DCCSI_PY_VERSION_MAJOR=3)
IF "%DCCSI_PY_VERSION_MINOR%"=="" (set DCCSI_PY_VERSION_MINOR=9)
IF "%DCCSI_PY_VERSION_RELEASE%"=="" (set DCCSI_PY_VERSION_RELEASE=7)

:: Default Maya Version
IF "%MAYA_VERSION%"=="" (set MAYA_VERSION=2023)

:: Initialize env
CALL %~dp0\Env_O3DE_Core.bat
CALL %~dp0\Env_O3DE_Python.bat

echo.
echo _____________________________________________________________________
echo.
echo ~    O3DE DCCsi Autodesk Maya Environment ...
echo _____________________________________________________________________
echo.

echo     DCCSI_PY_VERSION_MAJOR = %DCCSI_PY_VERSION_MAJOR%
echo     DCCSI_PY_VERSION_MINOR = %DCCSI_PY_VERSION_MINOR%
echo     DCCSI_PY_VERSION_RELEASE = %DCCSI_PY_VERSION_RELEASE%
echo     MAYA_VERSION = %MAYA_VERSION%

:::: Set Maya native project acess to this project
IF "%MAYA_PROJECT%"=="" (set MAYA_PROJECT=%PATH_O3DE_PROJECT%)
echo     MAYA_PROJECT = %MAYA_PROJECT%

:: Set dccsi maya tools path
set "PATH_DCCSI_TOOLS_DCC_MAYA=%PATH_DCCSI_TOOLS%\DCC\Maya"
echo     PATH_DCCSI_TOOLS_DCC_MAYA = %PATH_DCCSI_TOOLS_DCC_MAYA%

set MAYA_MODULE_PATH=%PATH_DCCSI_TOOLS_DCC_MAYA%;%MAYA_MODULE_PATH%
echo     MAYA_MODULE_PATH = %MAYA_MODULE_PATH%

:: Maya File Paths, etc
:: https://knowledge.autodesk.com/support/maya/learn-explore/caas/CloudHelp/cloudhelp/2015/ENU/Maya/files/Environment-Variables-File-path-variables-htm.html
:::: Set Maya native project acess to this project
IF "%MAYA_LOCATION%"=="" (set "MAYA_LOCATION=%ProgramFiles%\Autodesk\Maya%MAYA_VERSION%")
echo     MAYA_LOCATION = %MAYA_LOCATION%

IF "%MAYA_BIN_PATH%"=="" (set "MAYA_BIN_PATH=%MAYA_LOCATION%\bin")
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

:: shared location for 64bit DCCSI_PY_MAYA python location
set "DCCSI_PY_MAYA=%MAYA_BIN_PATH%\mayapy.exe"
echo     DCCSI_PY_MAYA = %DCCSI_PY_MAYA%

:: Local DCCsi Maya plugins access (ours)
set "DCCSI_MAYA_PLUG_IN_PATH=%PATH_DCCSI_TOOLS_DCC_MAYA%\plugins"
:: also attached to maya's built-it env var
set MAYA_PLUG_IN_PATH=%DCCSI_MAYA_PLUG_IN_PATH%;%MAYA_PLUG_IN_PATH%
echo     DCCSI_MAYA_PLUG_IN_PATH = %DCCSI_MAYA_PLUG_IN_PATH%

:: Local DCCsi Maya shelves (ours)
set "DCCSI_MAYA_SHELF_PATH=%PATH_DCCSI_TOOLS_DCC_MAYA%\Prefs\Shelves"
set MAYA_SHELF_PATH=%DCCSI_MAYA_SHELF_PATH%
echo     DCCSI_MAYA_SHELF_PATH = %DCCSI_MAYA_SHELF_PATH%

:: Local DCCsi Maya icons path (ours)
set "DCCSI_MAYA_XBMLANGPATH=%PATH_DCCSI_TOOLS_DCC_MAYA%\Prefs\icons"
:: also attached to maya's built-it env var
set XBMLANGPATH=%DCCSI_MAYA_XBMLANGPATH%;%XBMLANGPATH%
echo     DCCSI_MAYA_XBMLANGPATH = %DCCSI_MAYA_XBMLANGPATH%

:: Local DCCsi Maya Mel scripts (ours)
set "DCCSI_MAYA_SCRIPT_MEL_PATH=%PATH_DCCSI_TOOLS_DCC_MAYA%\Scripts\Mel"
:: also attached to maya's built-it env var
set MAYA_SCRIPT_PATH=%DCCSI_MAYA_SCRIPT_MEL_PATH%;%MAYA_SCRIPT_PATH%
echo     DCCSI_MAYA_SCRIPT_MEL_PATH = %DCCSI_MAYA_SCRIPT_MEL_PATH%

:: Local DCCsi Maya Python scripts (ours)
set "DCCSI_MAYA_SCRIPT_PY_PATH=%PATH_DCCSI_TOOLS_DCC_MAYA%\Scripts\Python"
:: also attached to maya's built-it env var
set MAYA_SCRIPT_PATH=%DCCSI_MAYA_SCRIPT_PY_PATH%;%MAYA_SCRIPT_PATH%
echo     DCCSI_MAYA_SCRIPT_PY_PATH = %DCCSI_MAYA_SCRIPT_PY_PATH%

:: DCCsi Maya boostrap, userSetup.py access (ours)
set "DCCSI_MAYA_SCRIPT_PATH=%PATH_DCCSI_TOOLS_DCC_MAYA%\Scripts"
:: also attached to maya's built-it env var
set MAYA_SCRIPT_PATH=%DCCSI_MAYA_SCRIPT_PATH%;%MAYA_SCRIPT_PATH%
echo     DCCSI_MAYA_SCRIPT_PATH = %DCCSI_MAYA_SCRIPT_PATH%
echo     MAYA_SCRIPT_PATH = %MAYA_SCRIPT_PATH%

:: DX11 Viewport
Set MAYA_VP2_DEVICE_OVERRIDE=VirtualDeviceDx11
echo     MAYA_VP2_DEVICE_OVERRIDE = %MAYA_VP2_DEVICE_OVERRIDE%

Set MAYA_SHOW_OUTPUT_WINDOW=True
echo     MAYA_SHOW_OUTPUT_WINDOW = %MAYA_SHOW_OUTPUT_WINDOW%

::ENDLOCAL

:: Set flag so we don't initialize dccsi environment twice
SET DCCSI_ENV_MAYA_INIT=1
GOTO END_OF_FILE

:: Return to starting directory
POPD

:END_OF_FILE
