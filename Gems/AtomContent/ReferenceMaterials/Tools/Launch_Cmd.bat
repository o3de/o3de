@echo off

REM 
REM Copyright (c) Contributors to the Open 3D Engine Project
REM 
REM SPDX-License-Identifier: Apache-2.0 OR MIT
REM For complete copyright and license terms please see the LICENSE at the root of this distribution.
REM
REM

:: Set up and start a O3DE CMD prompt
:: Sets up the current (DCC) Project_Env,
:: Puts you in the CMD within the dev environment

:: Set up window
TITLE O3DE Asset Gem Cmd
:: Use obvious color to prevent confusion (Grey with Yellow Text)
COLOR 8E

%~d0
cd %~dp0
PUSHD %~dp0

:: Keep changes local
SETLOCAL enableDelayedExpansion

CALL %~dp0\Project_Env.bat

echo.
echo _____________________________________________________________________
echo.
echo ~    O3DE Asset Gem CMD ...
echo _____________________________________________________________________
echo.

:: Create command prompt with environment
CALL %windir%\system32\cmd.exe

ENDLOCAL

:: Return to starting directory
POPD

:END_OF_FILE