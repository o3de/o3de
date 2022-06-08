@echo off
REM 
REM Copyright (c) Contributors to the Open 3D Engine Project.
REM For complete copyright and license terms please see the LICENSE at the root of this distribution.
REM
REM SPDX-License-Identifier: Apache-2.0 OR MIT
REM
REM

:: Sets up extended DCCsi environment for Adobe Substance Designer

:: Set up window
TITLE O3DE DCCsi Autodesk Substance Designer Environment
:: Use obvious color to prevent confusion (Grey with Yellow Text)
COLOR 8E

:: Skip initialization if already completed
IF "%DCCSI_ENV_SUBSTANCE_INIT%"=="1" GOTO :END_OF_FILE

:: Store current dir
%~d0
cd %~dp0
PUSHD %~dp0

:: Substance: Python 3.7.6 (tags/v3.7.6:43364a7ae0, Dec 19 2019, 00:42:30) [MSC v.1916 64 bit (AMD64)] on win32
IF "%DCCSI_PY_VERSION_MAJOR%"=="" (set DCCSI_PY_VERSION_MAJOR=3)
IF "%DCCSI_PY_VERSION_MINOR%"=="" (set DCCSI_PY_VERSION_MINOR=7)
IF "%DCCSI_PY_VERSION_RELEASE%"=="" (set DCCSI_PY_VERSION_RELEASE=6)

CALL %~dp0\Env_O3DE_Core.bat
CALL %~dp0\Env_O3DE_Python.bat

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
