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

:: Sets up environment for Lumberyard DCC tools and code access

:: Skip initialization if already completed
IF "%DCCSI_ENV_VSCODE_INIT%"=="1" GOTO :END_OF_FILE

:: Store current dir
%~d0
cd %~dp0
PUSHD %~dp0

:: Initialize env
CALL %~dp0\Env_Core.bat
CALL %~dp0\Env_Python.bat
CALL %~dp0\Env_Qt.bat

::SETLOCAL ENABLEDELAYEDEXPANSION

:: vscode has either a user install or a system install
:: https://code.visualstudio.com/#alt-downloads
:: that will change the paths assumed in this launcher (assume system install)
:: vscode envars: https://code.visualstudio.com/docs/editor/variables-reference

SET VSCODE_WRKSPC=%DCCSIG_PATH%\Solutions\.vscode\dccsi.code-workspace

echo.
echo _____________________________________________________________________
echo.
echo ~    O3DE DCCsi WingIDE Environment ...
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
