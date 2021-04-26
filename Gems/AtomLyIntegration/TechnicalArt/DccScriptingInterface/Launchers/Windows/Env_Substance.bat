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
IF "%DCCSI_ENV_SUBSTANCE_INIT%"=="1" GOTO :END_OF_FILE

:: Store current dir
%~d0
cd %~dp0
PUSHD %~dp0

CALL %~dp0\Env_Core.bat
CALL %~dp0\Env_Python.bat

::SETLOCAL ENABLEDELAYEDEXPANSION

echo.
echo _____________________________________________________________________
echo.
echo ~    O3DE DCCsi Adobe Substance Environment ...
echo _____________________________________________________________________
echo.

: Substance Designer
:: maya sdk path
set DCCSI_SUBSTANCE_PATH=%DCCSI_SDK_PATH%\Substance
echo     DCCSI_SUBSTANCE_PATH = %DCCSI_SUBSTANCE_PATH%
:: https://docs.substance3d.com/sddoc/project-preferences-107118596.html#ProjectPreferences-ConfigurationFile
:: Path to .exe, "C:\Program Files\Allegorithmic\Substance Designer\Substance Designer.exe"
set SUBSTANCE_PATH="%ProgramFiles%\Allegorithmic\Substance Designer"
echo     SUBSTANCE_PATH = %SUBSTANCE_PATH%

:: default config
set SUBSTANCE_CFG_PATH=%LY_PROJECT_PATH%\DCCsi_default.sbscfg
echo     SUBSTANCE_CFG_PATH = %SUBSTANCE_CFG_PATH%

::ENDLOCAL

:: Set flag so we don't initialize dccsi environment twice
SET DCCSI_ENV_SUBSTANCE_INIT=1
GOTO END_OF_FILE

:: Return to starting directory
POPD

:END_OF_FILE
