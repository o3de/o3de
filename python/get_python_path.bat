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

:: Note, many DCC tools (like Maya) include thier own versioned python interpretter.
:: Some apps may not operate correctly if PYTHONHOME is set/propogated.
:: This is definitely the case with Maya, doing so causes Maya to not boot.
FOR /F "tokens=* USEBACKQ" %%F IN (`%CMD_DIR%\python.cmd %CMD_DIR%\get_python_path.py`) DO (SET O3DE_PYTHONHOME=%%F)
echo     O3DE_PYTHONHOME - is now the folder containing O3DE python executable
echo     O3DE_PYTHONHOME = %O3DE_PYTHONHOME% 

SET PYTHON=%O3DE_PYTHONHOME%\python.exe

:: Set flag so we don't initialize dccsi environment twice
SET O3DE_PYTHONHOME_INIT=1
GOTO END_OF_FILE

:: Return to starting directory
POPD

:END_OF_FILE
