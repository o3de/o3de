@ECHO OFF
REM 
REM Copyright (c) Contributors to the Open 3D Engine Project.
REM For complete copyright and license terms please see the LICENSE at the root of this distribution.
REM
REM SPDX-License-Identifier: Apache-2.0 OR MIT
REM
REM

REM This script provides a single entry point that you can trust is present.
REM Depending on this entry point instead of trying to find a python.exe
REM In a subfolder allows you to keep working if the version of python changes or
REM other environmental requirements change.
REM When the project switches to a new version of Python, this file will be updated.

SETLOCAL
SET CMD_DIR=%~dp0
SET CMD_DIR=%CMD_DIR:~0,-1%

SET PYTHONHOME=%CMD_DIR%\runtime\python-3.10.5-rev1-windows\python

IF EXIST "%PYTHONHOME%" GOTO PYTHONHOME_EXISTS

ECHO Python not found in %CMD_DIR%
ECHO Try running %CMD_DIR%\get_python.bat first.
exit /B 1

:PYTHONHOME_EXISTS

SET PYTHON=%PYTHONHOME%\python.exe
SET PYTHON_ARGS=%*

IF [%1] EQU [debug] (
    SET PYTHON=%PYTHONHOME%\python_d.exe
    SET PYTHON_ARGS=%PYTHON_ARGS:~6%
)

IF EXIST "%PYTHON%" GOTO PYTHON_EXISTS

ECHO Could not find python executable at %PYTHON%
exit /B 1

:PYTHON_EXISTS

SET PYTHONNOUSERSITE=1
"%PYTHON%" %PYTHON_ARGS%
exit /B %ERRORLEVEL%
