@echo off
REM 
REM Copyright (c) Contributors to the Open 3D Engine Project.
REM For complete copyright and license terms please see the LICENSE at the root of this distribution.
REM
REM SPDX-License-Identifier: Apache-2.0 OR MIT
REM
REM

:: Set up and run LY Python CMD prompt
:: Sets up the DccScriptingInterface_Env,
:: Puts you in the CMD within the dev environment

:: Set up window
TITLE O3DE DCCsi Launch pyBase
:: Use obvious color to prevent confusion (Grey with Yellow Text)
COLOR 8E

%~d0
cd %~dp0
PUSHD %~dp0

SETLOCAL ENABLEDELAYEDEXPANSION

:: Initialize env
CALL %~dp0\Env_Core.bat
CALL %~dp0\Env_Python.bat
CALL %~dp0\Env_Qt.bat
CALL %~dp0\Env_Maya.bat

:: if the user has set up a custom env call it
IF EXIST "%~dp0Env_Dev.bat" CALL %~dp0Env_Dev.bat

echo Starting: %DCCSI_PY_BASE%

call %DCCSI_PY_BASE% %*

ENDLOCAL

:: Return to starting directory
POPD

:END_OF_FILE
