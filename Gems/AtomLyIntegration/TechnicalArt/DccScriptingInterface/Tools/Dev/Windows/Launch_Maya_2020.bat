@echo off
REM 
REM Copyright (c) Contributors to the Open 3D Engine Project.
REM For complete copyright and license terms please see the LICENSE at the root of this distribution.
REM
REM SPDX-License-Identifier: Apache-2.0 OR MIT
REM
REM

:: Store current dir
%~d0
cd %~dp0
PUSHD %~dp0

SETLOCAL ENABLEDELAYEDEXPANSION

:: Default Maya and Python version
set MAYA_VERSION=2020
set DCCSI_PY_VERSION_MAJOR=2
set DCCSI_PY_VERSION_MINOR=7
set DCCSI_PY_VERSION_RELEASE=11

CALL %~dp0\Env_Core.bat
CALL %~dp0\Env_Python.bat
CALL %~dp0\Env_Maya.bat

:: ide and debugger plug
set DCCSI_PY_DEFAULT=%DCCSI_PY_MAYA%

:: Default BASE DCCsi python 3.7 location
:: Can be overridden (example, Launch_mayaPy_2020.bat :: MayaPy.exe)
set DCCSI_PY_DCCSI=%DCCSI_LAUNCHERS_PATH%Launch_mayaPy_2020.bat
echo     DCCSI_PY_DCCSI = %DCCSI_PY_DCCSI%

:: if the user has set up a custom env call it
IF EXIST "%~dp0Env_Dev.bat" CALL %~dp0Env_Dev.bat

echo.
echo _____________________________________________________________________
echo.
echo Launching Maya %MAYA_VERSION% for O3DE DCCsi...
echo _____________________________________________________________________
echo.

echo     MAYA_VERSION = %MAYA_VERSION%
echo     DCCSI_PY_VERSION_MAJOR = %DCCSI_PY_VERSION_MAJOR%
echo     DCCSI_PY_VERSION_MINOR = %DCCSI_PY_VERSION_MINOR%
echo     DCCSI_PY_VERSION_RELEASE = %DCCSI_PY_VERSION_RELEASE%
echo     MAYA_LOCATION = %MAYA_LOCATION%
echo     MAYA_BIN_PATH = %MAYA_BIN_PATH%

:: Change to root dir
CD /D %O3DE_PROJECT_PATH%

:: Default to the right version of Maya if we can detect it... and launch
IF EXIST "%MAYA_BIN_PATH%\maya.exe" (
    start "" "%MAYA_BIN_PATH%\maya.exe" %*
) ELSE (
    Where maya.exe 2> NUL
    IF ERRORLEVEL 1 (
        echo Maya.exe could not be found
            pause
    ) ELSE (
        start "" Maya.exe %*
    )
)

::ENDLOCAL

:: Restore previous directory
POPD

:END_OF_FILE
