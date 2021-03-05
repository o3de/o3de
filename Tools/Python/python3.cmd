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

SETLOCAL
SET CMD_DIR=%~dp0
SET CMD_DIR=%CMD_DIR:~0,-1%

echo WARNING:  Using deprecated python3.sh in $DIR - please update your scripts
echo           to use python.sh in the python subfolder of the root instead.

rem we fetch python pre-emptively because the prior legacy system had
rem python pre-installed...
call %CMD_DIR%/../../python/get_python.bat

if ERRORLEVEL 1 (
    ECHO Failed to fetch python
    EXIT /b 1
)

call %CMD_DIR%/../../python/python.cmd %*
exit /b %ERRORLEVEL%
