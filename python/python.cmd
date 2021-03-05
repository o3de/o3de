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

REM This script provides a single entry point that you can trust is present.
REM Depending on this entry point instead of trying to find a python.exe
REM In a subfolder allows you to keep working if the version of python changes or
REM other environmental requirements change.
REM When the project switches to a new version of Python, this file will be updated.

SETLOCAL
SET CMD_DIR=%~dp0
SET CMD_DIR=%CMD_DIR:~0,-1%

SET PYTHONHOME=%CMD_DIR%\runtime\python-3.7.10-rev1-windows\python

IF EXIST "%PYTHONHOME%" GOTO PYTHONHOME_EXISTS

ECHO Could not find Python for Windows in %CMD_DIR%\..
exit /B 1

:PYTHONHOME_EXISTS

SET PYTHON=%PYTHONHOME%\python.exe
IF EXIST "%PYTHON%" GOTO PYTHON_EXISTS

ECHO Could not find python.exe in %PYTHONHOME%
exit /B 1

:PYTHON_EXISTS

SET PYTHONNOUSERSITE=1
"%PYTHON%" %*
exit /B %ERRORLEVEL%
