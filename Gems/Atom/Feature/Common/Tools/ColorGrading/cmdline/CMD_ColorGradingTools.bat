@echo off

REM 
REM Copyright (c) Contributors to the Open 3D Engine Project
REM 
REM SPDX-License-Identifier: Apache-2.0 OR MIT
REM For complete copyright and license terms please see the LICENSE at the root of this distribution.
REM
REM

:: Set up and run O3DE Python CMD prompt
:: Sets up the DccScriptingInterface_Env,
:: Puts you in the CMD within the dev environment

:: Set up window
TITLE O3DE Color Grading CMD
:: Use obvious color to prevent confusion (Grey with Yellow Text)
COLOR 8E

echo.
echo _____________________________________________________________________
echo.
echo ~    O3DE Color Grading Python CMD ...
echo _____________________________________________________________________
echo.

%~d0
cd %~dp0
PUSHD %~dp0

SETLOCAL ENABLEDELAYEDEXPANSION

:: if the user has set up a custom env call it
IF EXIST "%~dp0User_Env.bat" CALL %~dp0User_Env.bat

:: Initialize env
echo.
echo     ... calling Env_Core.bat
CALL %~dp0\Env_Core.bat

echo.
echo     ... calling Env_Python.bat
CALL %~dp0\Env_Python.bat

echo.
echo     ... calling Env_Tools.bat
CALL %~dp0\Env_Tools.bat

:: Change to root dir
CD /D ..

:: Create command prompt with environment
CALL %windir%\system32\cmd.exe

ENDLOCAL

:: Return to starting directory
POPD

:END_OF_FILE
