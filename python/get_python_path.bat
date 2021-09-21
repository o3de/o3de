@echo off
REM 
REM Copyright (c) Contributors to the Open 3D Engine Project.
REM For complete copyright and license terms please see the LICENSE at the root of this distribution.
REM
REM SPDX-License-Identifier: Apache-2.0 OR MIT
REM
REM

:: Retreives the path to the O3DE python executable(s)

:: Skip initialization if already completed
IF "%O3DE_PYTHONHOME_INIT%"=="1" GOTO :END_OF_FILE

SET CMD_DIR=%~dp0

FOR /F "tokens=* USEBACKQ" %%F IN (`%CMD_DIR%\python.cmd %CMD_DIR%\get_python_path.py`) DO (SET PYTHONHOME=%%F)
echo     PYTHONHOME - is now the folder containing O3DE python executable
echo     PYTHONHOME = %PYTHONHOME% 

SET PYTHON=%PYTHONHOME%\python.exe

:: Set flag so we don't initialize dccsi environment twice
SET O3DE_PYTHONHOME_INIT=1
GOTO END_OF_FILE

:: Return to starting directory
POPD

:END_OF_FILE
