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

:: if the user has set up a custom env call it
IF EXIST "%~dp0..\Env_Dev.bat" CALL %~dp0..\Env_Dev.bat

:: Default Maya and Python version
set MAYA_VERSION=2022
set DCCSI_PY_VERSION_MAJOR=3
set DCCSI_PY_VERSION_MINOR=7
set DCCSI_PY_VERSION_RELEASE=7

CALL %~dp0..\Env_O3DE_Core.bat

:: add to the PATH here (this is global)
SET PATH=%PATH_O3DE_BIN%;%PATH_DCCSIG%;%PATH%

CALL %~dp0..\Env_DCC_Maya.bat

:: ide and debugger plug
set "DCCSI_PY_DEFAULT=%MAYA_BIN_PATH%\mayapy.exe"
echo     DCCSI_PY_DEFAULT = %DCCSI_PY_DEFAULT%

:: Some IDEs like Wing, may in some cases need acess directly to the exe to operate correctly
set "DCCSI_PY_IDE=%MAYA_BIN_PATH%\mayapy.exe"
echo     DCCSI_PY_IDE = %DCCSI_PY_IDE%

SET PATH=%MAYA_BIN_PATH%;%DCCSI_PY_IDE%;%DCCSI_PY_DEFAULT%;%PATH%

CALL %~dp0..\Env_O3DE_Python.bat

:: add to the PATH
SET PATH=%MAYA_BIN_PATH%;%PATH%

:: add all python related paths to PYTHONPATH for package imports
set PYTHONPATH=%DCCSI_MAYA_SCRIPT_PATH%;%PATH_O3DE_TECHART_GEMS%;%PATH_DCCSIG%;%PATH_DCCSI_PYTHON_LIB%;%PATH_O3DE_BIN%;%PYTHONPATH%

:: if the user has set up a custom env call it
IF EXIST "%~dp0..\Env_Dev.bat" CALL %~dp0..\Env_Dev.bat

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

echo.
echo     PATH = %PATH%
echo.
echo     PYTHONPATH = %PYTHONPATH%
echo.

:: Change to root dir
CD /D %PATH_O3DE_PROJECT%

SETLOCAL ENABLEDELAYEDEXPANSION

:: Default to the right version of Maya if we can detect it... and launch
IF EXIST "%DCCSI_PY_MAYA%" (
    start "" "%DCCSI_PY_MAYA%" %*
) ELSE (
    Where maya.exe 2> NUL
    IF ERRORLEVEL 1 (
        echo MayaPy.exe could not be found
            pause
    ) ELSE (
        start "" MayaPy.exe %*
    )
)

ENDLOCAL

:: Return to starting directory
POPD

:END_OF_FILE
