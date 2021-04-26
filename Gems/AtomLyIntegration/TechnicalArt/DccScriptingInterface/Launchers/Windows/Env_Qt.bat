@echo off
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

:: Skip initialization of QT if already completed
IF "%DCCSI_ENV_QT_INIT%"=="1" GOTO :END_OF_FILE

:: Store current dir
%~d0
cd %~dp0
PUSHD %~dp0

CALL %~dp0\Env_Core.bat
CALL %~dp0\Env_Python.bat

::SETLOCAL ENABLEDELAYEDEXPANSION

echo.
echo _____________________________________________________________________
echo.
echo ~    O3DE DCCsi Qt/PySide2 Environment ...
echo _____________________________________________________________________
echo.

:: These need to be ADDED to any env or launcher that needs Qt/PySide2 access
:: This bootstraps O3DE Qt binaraies and PySide2 packages (which likely won't work in other versions of python)
:: If you set these in the env.bat it may cause some Qt apps like WingIDE from starting correctly (Wing is a Qt app)
:: Those apps provide their own Qt bins and Pyside packages (Wing, Substance, Maya, etc.)
:: set up Qt/Pyside paths

:: set up PySide2/Shiboken 
set QTFORPYTHON_PATH=%LY_DEV%\Gems\QtForPython\3rdParty\pyside2\windows\release
echo     QTFORPYTHON_PATH = %QTFORPYTHON_PATH%

:: add to the PATH
SET PATH=%QTFORPYTHON_PATH%;%PATH%
SET PYTHONPATH=%QTFORPYTHON_PATH%;%PYTHONPATH%

set QT_PLUGIN_PATH=%LY_BUILD_PATH%\bin\profile\EditorPlugins
echo     QT_PLUGIN_PATH = %QT_PLUGIN_PATH%

:: add to the PATH
SET PATH=%QT_PLUGIN_PATH%;%PATH%
SET PYTHONPATH=%QT_PLUGIN_PATH%;%PYTHONPATH%

set LY_BIN_PATH=%LY_BUILD_PATH%\bin\profile
echo     LY_BIN_PATH = %LY_BIN_PATH%
SET PATH=%LY_BIN_PATH%;%PATH%

::ENDLOCAL

:: Set flag so we don't initialize dccsi environment twice
SET DCCSI_ENV_QT_INIT=1
GOTO END_OF_FILE

:: Return to starting directory
POPD

:END_OF_FILE
