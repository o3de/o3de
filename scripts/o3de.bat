@ECHO OFF
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

pushd %~dp0%
CD %~dp0..
SET BASE_PATH=%CD%
CD %~dp0
SET PYTHON_DIRECTORY=%BASE_PATH%\python
IF EXIST "%PYTHON_DIRECTORY%" GOTO pythonPathAvailable
GOTO pythonDirNotFound
:pythonPathAvailable
SET PYTHON_EXECUTABLE=%PYTHON_DIRECTORY%\python.cmd
IF NOT EXIST "%PYTHON_EXECUTABLE%" GOTO pythonExeNotFound
CALL "%PYTHON_EXECUTABLE%" %BASE_PATH%\scripts\o3de.py %*
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


