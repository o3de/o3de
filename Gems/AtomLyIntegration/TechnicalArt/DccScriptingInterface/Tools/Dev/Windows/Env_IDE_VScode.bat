@echo off
REM 
REM Copyright (c) Contributors to the Open 3D Engine Project.
REM For complete copyright and license terms please see the LICENSE at the root of this distribution.
REM
REM SPDX-License-Identifier: Apache-2.0 OR MIT
REM
REM

:: Sets up environment for O3DE DCC tools and code access

:: Skip initialization if already completed
IF "%DCCSI_ENV_VSCODE_INIT%"=="1" GOTO :END_OF_FILE

:: Store current dir
%~d0
cd %~dp0
PUSHD %~dp0

:: Initialize env
CALL %~dp0\Env_O3DE_Core.bat
CALL %~dp0\Env_O3DE_Python.bat
CALL %~dp0\Env_O3DE_Qt.bat

::SETLOCAL ENABLEDELAYEDEXPANSION

:: vscode has either a user install or a system install
:: https://code.visualstudio.com/#alt-downloads
:: that will change the paths assumed in this launcher (assume system install)
:: vscode envars: https://code.visualstudio.com/docs/editor/variables-reference

IF "%VSCODEHOME%"=="" (SET "VSCODEHOME=%ProgramFiles%\Microsoft VS Code\")

IF "%VSCODE_WRKSPC%"=="" (SET "VSCODE_WRKSPC=%PATH_DCCSIG%\Tools\Dev\Windows\Solutions\.vscode\dccsi.code-workspace")

echo.
echo _____________________________________________________________________
echo.
echo ~    O3DE DCCsi VScode Environment ...
echo _____________________________________________________________________
echo.

echo     VSCODE_WRKSPC = %VSCODE_WRKSPC%

::ENDLOCAL

:: Set flag so we don't initialize dccsi environment twice
SET DCCSI_ENV_VSCODE_INIT=1
GOTO END_OF_FILE

:: Return to starting directory
POPD

:END_OF_FILE
