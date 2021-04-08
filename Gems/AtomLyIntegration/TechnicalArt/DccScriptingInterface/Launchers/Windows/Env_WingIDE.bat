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

:: Wing and other IDEs probably prefer access directly to the python.exe
set DCCSI_PY_IDE = %DCCSI_PYTHON_INSTALL%\runtime\python-3.7.10-rev1-windows\python
echo     DCCSI_PY_IDE = %DCCSI_PY_IDE%

:: ide and debugger plug
set DCCSI_PY_DEFAULT=%DCCSI_PY_IDE%\python.exe

:: put project env variables/paths here
set WINGHOME=%PROGRAMFILES(X86)%\Wing Pro %DCCSI_WING_VERSION_MAJOR%.%DCCSI_WING_VERSION_MINOR%
SET WING_PROJ=%DCCSIG_PATH%\Solutions\.wing\DCCsi_%DCCSI_WING_VERSION_MAJOR%x.wpr

::SETLOCAL ENABLEDELAYEDEXPANSION

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

::ENDLOCAL

:: Set flag so we don't initialize dccsi environment twice
SET DCCSI_ENV_WINGIDE_INIT=1
GOTO END_OF_FILE

:: Return to starting directory
POPD

:END_OF_FILE