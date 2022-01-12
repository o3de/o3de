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
set "PATH_DCCSI_SUBSTANCE=%PATH_DCCSI_TOOLS%\Substance"
echo     PATH_DCCSI_SUBSTANCE = %PATH_DCCSI_SUBSTANCE%
:: https://docs.substance3d.com/sddoc/project-preferences-107118596.html#ProjectPreferences-ConfigurationFile
:: Path to .exe, "C:\Program Files\Allegorithmic\Substance Designer\Substance Designer.exe"
set "PATH_SUBSTANCE_DESIGNER=%ProgramFiles%\Allegorithmic\Substance Designer"
echo     PATH_SUBSTANCE_DESIGNER = %PATH_SUBSTANCE_DESIGNER%

:: default config
IF "%PATH_SUBSTANCE_DESIGNER_CFG%"=="" (set "PATH_SUBSTANCE_DESIGNER_CFG=%PATH_O3DE_PROJECT%\DCCsi_default.sbscfg")
echo     PATH_SUBSTANCE_DESIGNER_CFG = %PATH_SUBSTANCE_DESIGNER_CFG%

::ENDLOCAL

:: Set flag so we don't initialize dccsi environment twice
SET DCCSI_ENV_SUBSTANCE_INIT=1
GOTO END_OF_FILE

:: Return to starting directory
POPD

:END_OF_FILE
