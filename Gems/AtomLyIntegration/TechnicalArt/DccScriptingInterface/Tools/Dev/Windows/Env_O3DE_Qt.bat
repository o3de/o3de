@echo off
REM
REM Copyright (c) Contributors to the Open 3D Engine Project.
REM For complete copyright and license terms please see the LICENSE at the root of this distribution.
REM
REM SPDX-License-Identifier: Apache-2.0 OR MIT
REM
REM

:: Skip initialization of QT if already completed
IF "%DCCSI_ENV_QT_INIT%"=="1" GOTO :END_OF_FILE

:: Store current dir
%~d0
cd %~dp0
PUSHD %~dp0

CALL %~dp0\Env_O3DE_Core.bat
CALL %~dp0\Env_O3DE_Python.bat

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
:: notice: the way Qy/PySide2/shiboken2 are set up has changed for release 2210

set "QT_BIN_DIR=packages\qt-5.15.2-rev7-windows\qt\bin"

IF "%QT_PLUGIN_PATH%"=="" (set "QT_PLUGIN_PATH=%PATH_O3DE_3RDPARTY%\%QT_BIN_DIR%")
echo     QT_PLUGIN_PATH = %QT_PLUGIN_PATH%

set "QTPY_PKG_DIR=packages\pyside2-5.15.2.1-py3.10-rev3-windows\pyside2\lib\site-packages"

IF "%QTFORPYTHON_PATH%"=="" (set "QTFORPYTHON_PATH=%PATH_O3DE_3RDPARTY%\%QTPY_PKG_DIR%")
echo     QTFORPYTHON_PATH = %QTFORPYTHON_PATH%

::ENDLOCAL

:: Set flag so we don't initialize dccsi environment twice
SET DCCSI_ENV_QT_INIT=1
GOTO END_OF_FILE

:: Return to starting directory
POPD

:END_OF_FILE
