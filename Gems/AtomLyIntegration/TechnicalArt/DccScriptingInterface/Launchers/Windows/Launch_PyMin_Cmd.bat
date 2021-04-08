@echo off

REM 
REM All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
REM its licensors.
REM
REM For complete copyright and license terms please see the LICENSE at the root of this
REM distribution (the "License"). All use of this software is governed by the License,
REM or, if provided, by the license below or the license accompanying this file. Do not
REM remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
REM WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
REM

:: Set up and run LY Python CMD prompt
:: Sets up the DccScriptingInterface_Env,
:: Puts you in the CMD within the dev environment

:: Set up window
TITLE O3DE DCC Scripting Interface Py Min CMD
:: Use obvious color to prevent confusion (Grey with Yellow Text)
COLOR 8E

%~d0
cd %~dp0
PUSHD %~dp0

SETLOCAL ENABLEDELAYEDEXPANSION

:: Initialize env
CALL %~dp0\Env_Core.bat
CALL %~dp0\Env_Python.bat

:: if the user has set up a custom env call it
IF EXIST "%~dp0Env_Dev.bat" CALL %~dp0Env_Dev.bat

echo.
echo _____________________________________________________________________
echo.
echo ~    O3DE DCC Scripting Interface, Py Min Env CMD ...
echo _____________________________________________________________________
echo.

:: Change to root dir
CD /D %LY_PROJECT_PATH%

:: Create command prompt with environment
CALL %windir%\system32\cmd.exe

ENDLOCAL

:: Return to starting directory
POPD

:END_OF_FILE