@ECHO OFF
REM 
REM Copyright (c) Contributors to the Open 3D Engine Project.
REM For complete copyright and license terms please see the LICENSE at the root of this distribution.
REM
REM SPDX-License-Identifier: Apache-2.0 OR MIT
REM
REM

setlocal enabledelayedexpansion 

SET CMD_DIR=%~dp0
SET CMD_DIR=%CMD_DIR:~0,-1%


REM General strategy: Check if python is already installed
REM If not, use cmake and the new package system to install it.

REM Is python installed?
cd /D %CMD_DIR%
call python.cmd --version > NUL
IF !ERRORLEVEL!==0 (
    echo get_python.bat: Python is already installed:
    call python.cmd --version
    call "%CMD_DIR%\pip.cmd" install -r "%CMD_DIR%/requirements.txt" --quiet --disable-pip-version-check --no-warn-script-location
    call "%CMD_DIR%\pip.cmd" install -e "%CMD_DIR%/../scripts/o3de" --quiet --disable-pip-version-check --no-warn-script-location --no-deps
    exit /B 0
)

cd /D %CMD_DIR%\..
REM IF you update this logic, update it in scripts/build/Platform/Windows/env_windows.cmd
REM If cmake is not found on path, try a known location at LY_CMAKE_PATH
where /Q cmake
IF NOT !ERRORLEVEL!==0 (
    IF "%LY_CMAKE_PATH%"=="" (
        ECHO ERROR: CMake was not found on the PATH and LY_CMAKE_PATH is not defined.
        ECHO Please ensure CMake is on the path or set LY_CMAKE_PATH.
        EXIT /b 1
    )

    PATH !LY_CMAKE_PATH!;!PATH!
    where /Q cmake
    if NOT !ERRORLEVEL!==0 (
        ECHO ERROR: CMake was not found on the PATH or at the known location: !LY_CMAKE_PATH!
        ECHO Please add it to the path, set LY_CMAKE_PATH to be the directory containing it, or place it
        ECHO at the above location.
        EXIT /b 1
    )
)

REM output the version number for forensic logging
cmake --version
cmake -DPAL_PLATFORM_NAME:string=Windows -D "LY_3RDPARTY_PATH:string=%CMD_DIR%" -P "%CMD_DIR%\get_python.cmake"

if ERRORLEVEL 1 (
    ECHO ERROR: Unable to fetch python using cmake.  
    ECHO  - Is LY_PACKAGE_SERVER_URLS set?  
    ECHO  - Do you have permission to access the packages?
    EXIT /b 1
)

echo calling PIP to install requirements...
call "%CMD_DIR%\pip.cmd" install -r "%CMD_DIR%/requirements.txt" --disable-pip-version-check --no-warn-script-location
call "%CMD_DIR%\pip.cmd" install -e "%CMD_DIR%/../scripts/o3de" --disable-pip-version-check --no-warn-script-location --no-deps
exit /B %ERRORLEVEL%

