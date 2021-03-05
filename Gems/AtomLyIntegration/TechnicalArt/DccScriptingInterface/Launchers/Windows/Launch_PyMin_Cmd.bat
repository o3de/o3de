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
TITLE Lumberyard DCC Scripting Interface Cmd
:: Use obvious color to prevent confusion (Grey with Yellow Text)
COLOR 8E

%~d0
cd %~dp0

:: Keep changes local
SETLOCAL enableDelayedExpansion

:: This maps up to the \Dev folder
IF "%DEV_REL_PATH%"=="" (set DEV_REL_PATH=..\..\..\..\..\..\)

:: Change to root Lumberyard dev dir
:: Don't use the LY_DEV so we can test that ENVAR!!!
CD /d %DEV_REL_PATH%
set Rel_Dev=%CD%
echo     Rel_Dev = %Rel_Dev%
:: Restore original directory
popd

set DCCSI_PYTHON_INSTALL=%Rel_Dev%\Tools\Python\3.7.5\windows

:: add to the PATH
SET PATH=%DCCSI_PYTHON_INSTALL%;%PATH%

set DCCSI_PY_BASE=%DCCSI_PYTHON_INSTALL%\python.exe
echo     DCCSI_PY_BASE = %DCCSI_PY_BASE%

echo.
echo _____________________________________________________________________
echo.
echo ~    LY DCC Scripting Interface, Py Min Env CMD ...
echo _____________________________________________________________________
echo.

:: Create command prompt with environment
CALL %windir%\system32\cmd.exe

ENDLOCAL

:: Return to starting directory
POPD

:END_OF_FILE