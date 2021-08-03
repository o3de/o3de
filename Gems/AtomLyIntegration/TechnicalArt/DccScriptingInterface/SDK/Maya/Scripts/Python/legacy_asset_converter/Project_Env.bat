:: coding:utf-8
:: !/usr/bin/python
::
:: Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
:: 
:: SPDX-License-Identifier: Apache-2.0 OR MIT
::
::

@echo off
:: Sets up environment for Lumberyard DCC tools and code access

:: Store current dir
%~d0
cd %~dp0
PUSHD %~dp0

for %%a in (.) do set LY_PROJECT=%%~na

echo.
echo _____________________________________________________________________
echo.
echo ~    Setting up LY DSI PROJECT Environment ...
echo _____________________________________________________________________
echo.

echo     LY_PROJECT = %LY_PROJECT%

:: Put you project env vars and overrides here

:: chanhe the relative path up to dev
set DEV_REL_PATH=../../..
set ABS_PATH=%~dp0

:: Override the default maya version
set MAYA_VERSION=2020
echo     MAYA_VERSION = %MAYA_VERSION%

set LY_PROJECT_PATH=%ABS_PATH%
echo     LY_PROJECT_PATH = %LY_PROJECT_PATH%

:: Change to root Lumberyard dev dir
CD /d %LY_PROJECT_PATH%\%DEV_REL_PATH%
set LY_DEV=%CD%
echo     LY_DEV = %LY_DEV%

CALL %LY_DEV%\Gems\AtomLyIntegration\TechnicalArt\DccScriptingInterface\Launchers\Windows\Env.bat

rem :: Constant Vars (Global)
rem SET LYPY_GDEBUG=0
rem echo     LYPY_GDEBUG = %LYPY_GDEBUG%
rem SET LYPY_DEV_MODE=0
rem echo     LYPY_DEV_MODE = %LYPY_DEV_MODE%
rem SET LYPY_DEBUGGER=WING
rem echo     LYPY_DEBUGGER = %LYPY_DEBUGGER%

:: Restore original directory
popd

:: Change to root dir
CD /D %ABS_PATH%

:: if the user has set up a custom env call it
IF EXIST "%~dp0User_Env.bat" CALL %~dp0User_Env.bat

GOTO END_OF_FILE

:: Return to starting directory
POPD

:END_OF_FILE
