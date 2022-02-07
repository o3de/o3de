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
IF "%DCCSI_ENV_WINGIDE_INIT%"=="1" GOTO :END_OF_FILE

:: Store current dir
%~d0
cd %~dp0
PUSHD %~dp0

:: WingIDE version Major
IF "%DCCSI_WING_VERSION_MAJOR%"=="" (set DCCSI_WING_VERSION_MAJOR=7)
:: WingIDE version Major
IF "%DCCSI_WING_VERSION_MINOR%"=="" (set DCCSI_WING_VERSION_MINOR=1)

:: Initialize env
CALL %~dp0\Env_Core.bat
CALL %~dp0\Env_Python.bat
CALL %~dp0\Env_Qt.bat

:: put project env variables/paths here
set WINGHOME=%PROGRAMFILES(X86)%\Wing Pro %DCCSI_WING_VERSION_MAJOR%.%DCCSI_WING_VERSION_MINOR%
SET WING_PROJ=%DCCSIG_PATH%\Tools\Dev\Windows\Solutions\.wing\DCCsi_%DCCSI_WING_VERSION_MAJOR%x.wpr

echo.
echo _____________________________________________________________________
echo.
echo ~    O3DE DCCsi WingIDE Environment ...
echo _____________________________________________________________________
echo.

echo     DCCSI_WING_VERSION_MAJOR = %DCCSI_WING_VERSION_MAJOR%
echo     DCCSI_WING_VERSION_MINOR = %DCCSI_WING_VERSION_MINOR%
echo     WINGHOME = %WINGHOME%
echo     WING_PROJ = %WING_PROJ%

:: add to the PATH
SET PATH=%WINGHOME%;%PATH%

:: Set flag so we don't initialize dccsi environment twice
SET DCCSI_ENV_WINGIDE_INIT=1
GOTO END_OF_FILE

:: Return to starting directory
POPD

:END_OF_FILE
