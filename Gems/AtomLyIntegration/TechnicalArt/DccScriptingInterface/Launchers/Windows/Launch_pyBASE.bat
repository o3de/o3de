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

:: Set up and run LY Python CMD prompt
:: Sets up the DccScriptingInterface_Env,
:: Puts you in the CMD within the dev environment

:: Set up window
TITLE DCCsi (miniconda3)
:: Use obvious color to prevent confusion (Grey with Yellow Text)
COLOR 8E

%~d0
cd %~dp0
PUSHD %~dp0

:: Keep changes local
:: SETLOCAL enableDelayedExpansion

CALL %~dp0\Env.bat

:: These need to be ADDED to any env or launcher that is py3.7+ and needs PySide2 access
:: This bootstraps LUMBERYARDS Qt binaraies and PySide2 packages (which likely won't work in other versions of python)
:: If you set these in the env.bat it will cause some Qt apps like WingIDE from starting correctly
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

echo Starting: %DCCSI_PYTHON_INSTALL%\python.exe

call %DCCSI_PYTHON_INSTALL%\python.exe %*