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
IF "%DCCSI_ENV_PYCHARM_INIT%"=="1" GOTO :END_OF_FILE

:: Store current dir
%~d0
cd %~dp0
PUSHD %~dp0

:: version Year
IF "%PYCHARM_VERSION_YEAR%"=="" (set PYCHARM_VERSION_YEAR=2020)
:: version Major
IF "%PYCHARM_VERSION_MAJOR%"=="" (set PYCHARM_VERSION_MAJOR=3)
:: version Minor
IF "%PYCHARM_VERSION_MINOR%"=="" (set PYCHARM_VERSION_MINOR=2)

::"C:\Program Files\JetBrains\PyCharm 2019.1.3\bin"
::"C:\Program Files\JetBrains\PyCharm Community Edition 2018.3.5\bin\pycharm64.exe"

:: put project env variables/paths here
set PYCHARM_HOME=%PROGRAMFILES%\JetBrains\PyCharm %PYCHARM_VERSION_YEAR%.%PYCHARM_VERSION_MAJOR%.%PYCHARM_VERSION_MINOR%

:: Initialize env
CALL %~dp0\Env_Core.bat
CALL %~dp0\Env_Python.bat
CALL %~dp0\Env_Qt.bat

SET PYCHARM_PROJ=%PATH_DCCSIG%\Tools\Dev\Windows\Solutions

echo.
echo _____________________________________________________________________
echo.
echo ~    Setting up O3DE DCCsi Dev Env PyCharm %PYCHARM_VERSION_YEAR%.%PYCHARM_VERSION_MAJOR%.%PYCHARM_VERSION_MINOR%
echo _____________________________________________________________________
echo.

echo     PYCHARM_VERSION_YEAR = %PYCHARM_VERSION_YEAR%
echo     PYCHARM_VERSION_MAJOR = %PYCHARM_VERSION_MAJOR%
echo     PYCHARM_HOME = %PYCHARM_HOME%
echo     PYCHARM_PROJ = %PYCHARM_PROJ%

:: Set flag so we don't initialize dccsi environment twice
SET DCCSI_ENV_PYCHARM_INIT=1
GOTO END_OF_FILE

:: Return to starting directory
POPD

:END_OF_FILE
