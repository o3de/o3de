@echo off
REM 
REM Copyright (c) Contributors to the Open 3D Engine Project.
REM For complete copyright and license terms please see the LICENSE at the root of this distribution.
REM
REM SPDX-License-Identifier: Apache-2.0 OR MIT
REM
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
set DCCSI_SUBSTANCE_PATH=%DCCSI_TOOLS_PATH%\Substance
echo     DCCSI_SUBSTANCE_PATH = %DCCSI_SUBSTANCE_PATH%
:: https://docs.substance3d.com/sddoc/project-preferences-107118596.html#ProjectPreferences-ConfigurationFile
:: Path to .exe, "C:\Program Files\Allegorithmic\Substance Designer\Substance Designer.exe"
set SUBSTANCE_PATH="%ProgramFiles%\Allegorithmic\Substance Designer"
echo     SUBSTANCE_PATH = %SUBSTANCE_PATH%

:: default config
set SUBSTANCE_CFG_PATH=%O3DE_PROJECT_PATH%\DCCsi_default.sbscfg
echo     SUBSTANCE_CFG_PATH = %SUBSTANCE_CFG_PATH%

::ENDLOCAL

:: Set flag so we don't initialize dccsi environment twice
SET DCCSI_ENV_SUBSTANCE_INIT=1
GOTO END_OF_FILE

:: Return to starting directory
POPD

:END_OF_FILE
