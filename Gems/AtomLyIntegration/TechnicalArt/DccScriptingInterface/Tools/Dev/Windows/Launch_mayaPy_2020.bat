@echo off
REM 
REM Copyright (c) Contributors to the Open 3D Engine Project.
REM For complete copyright and license terms please see the LICENSE at the root of this distribution.
REM
REM SPDX-License-Identifier: Apache-2.0 OR MIT
REM
REM

:: Set up window
TITLE O3DE DCCsi Launch MayaPy
:: Use obvious color to prevent confusion (Grey with Yellow Text)
COLOR 8E

:: Store current directory and change to environment directory so script works in any path.
%~d0
cd %~dp0
PUSHD %~dp0

:: Default Maya and Python version
set MAYA_VERSION=2020
set DCCSI_PY_VERSION_MAJOR=2
set DCCSI_PY_VERSION_MINOR=7
set DCCSI_PY_VERSION_RELEASE=11

CALL %~dp0\Env_Core.bat
CALL %~dp0\Env_Python.bat
CALL %~dp0\Env_Maya.bat

:: if the user has set up a custom env call it
IF EXIST "%~dp0Env_Dev.bat" CALL %~dp0Env_Dev.bat

:: ide and debugger plug
set DCCSI_PY_DEFAULT=%DCCSI_PY_MAYA%

echo.
echo _____________________________________________________________________
echo.
echo ~    Launching O3DE DCCsi MayaPy (%MAYA_VERSION%) ...
echo ________________________________________________________________
echo.

echo     MAYA_VERSION = %MAYA_VERSION%
echo     DCCSI_PY_VERSION_MAJOR = %DCCSI_PY_VERSION_MAJOR%
echo     DCCSI_PY_VERSION_MINOR = %DCCSI_PY_VERSION_MINOR%
echo     DCCSI_PY_VERSION_RELEASE = %DCCSI_PY_VERSION_RELEASE%
echo     MAYA_LOCATION = %MAYA_LOCATION%
echo     MAYA_BIN_PATH = %MAYA_BIN_PATH%

:: Change to root dir
CD /D %O3DE_PROJECT_PATH%

SETLOCAL ENABLEDELAYEDEXPANSION

:: Default to the right version of Maya if we can detect it... and launch
IF EXIST "%DCCSI_PY_MAYA%" (
    start "" "%DCCSI_PY_MAYA%" %*
) ELSE (
    Where maya.exe 2> NUL
    IF ERRORLEVEL 1 (
        echo Maya.exe could not be found
            pause
    ) ELSE (
        start "" Maya.exe %*
    )
)

ENDLOCAL

:: Return to starting directory
POPD

:END_OF_FILE
