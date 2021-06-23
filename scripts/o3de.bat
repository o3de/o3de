@ECHO OFF
REM
REM Copyright (c) Contributors to the Open 3D Engine Project
REM 
REM SPDX-License-Identifier: Apache-2.0 OR MIT
REM
REM

pushd %~dp0%
CD %~dp0..
SET "BASE_PATH=%CD%"
CD %~dp0
SET "PYTHON_DIRECTORY=%BASE_PATH%\python"
IF EXIST "%PYTHON_DIRECTORY%" GOTO pythonPathAvailable
GOTO pythonDirNotFound
:pythonPathAvailable
SET PYTHON_EXECUTABLE=%PYTHON_DIRECTORY%\python.cmd
IF NOT EXIST "%PYTHON_EXECUTABLE%" GOTO pythonExeNotFound
CALL "%PYTHON_EXECUTABLE%" "%BASE_PATH%\scripts\o3de.py" %*
GOTO end
:pythonDirNotFound
ECHO Python directory not found: %PYTHON_DIRECTORY%
GOTO fail
:pythonExeNotFound
ECHO Python executable not found: %PYTHON_EXECUTABLE%
GOTO fail
:fail
popd
EXIT /b 1
:end
popd
EXIT /b %ERRORLEVEL%


