@echo off
REM
REM Copyright (c) Contributors to the Open 3D Engine Project.
REM For complete copyright and license terms please see the LICENSE at the root of this distribution.
REM
REM SPDX-License-Identifier: Apache-2.0 OR MIT
REM
REM

:: Launches Maya bootstrapped with O3DE and DccScriptingInterface
:: Status: Prototype
:: Version: 0.0.1
:: Support: Maya 2022 and 2023 (python 3)
:: Readme.md:  https://github.com/o3de/o3de/tree/development/Gems/AtomLyIntegration/TechnicalArt/DccScriptingInterface/Tools/DCC/Maya
:: Notes:
:: - If you have a non-standard default install location for Maya, you may need  to do manual configuration
:: - Try overriding envars and paths in your Env_Dev.bat

:: Set up window
TITLE O3DE DCCsi Launch Maya
:: Use obvious color to prevent confusion (Grey with Yellow Text)
COLOR 8E

echo.
echo _____________________________________________________________________
echo.
echo ~    O3DE DCCsi Maya Launch Env ...
echo _____________________________________________________________________
echo.
echo ~    default envas for Maya env
echo.

:: Store current dir
%~d0
cd %~dp0
PUSHD %~dp0

SETLOCAL ENABLEDELAYEDEXPANSION

:: Default Maya and Python version
set MAYA_VERSION=2022
set DCCSI_PY_VERSION_MAJOR=3
set DCCSI_PY_VERSION_MINOR=7
set DCCSI_PY_VERSION_RELEASE=7

:: if the user has set up a custom env call it
:: this ensures any user envars remain intact after initializing other envs
IF EXIST "%~dp0win_launch_maya.bat" CALL %~dp0win_launch_maya.bat

::ENDLOCAL

:: Restore previous directory
POPD

:END_OF_FILE
