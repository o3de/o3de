@echo off
REM 
REM Copyright (c) Contributors to the Open 3D Engine Project.
REM For complete copyright and license terms please see the LICENSE at the root of this distribution.
REM
REM SPDX-License-Identifier: Apache-2.0 OR MIT
REM
REM

:: Set up and run O3DE Python CMD prompt
:: Sets up the DccScriptingInterface_Env,
:: Puts you in the CMD within the dev environment

:: Set up window
TITLE O3DE DCC Scripting Interface Cmd
:: Use obvious color to prevent confusion (Grey with Yellow Text)
COLOR 8E

echo.
echo _____________________________________________________________________
echo.
echo ~    O3DE DCCsi DCCSI_PY_BASE (python) CMD ...
echo _____________________________________________________________________
echo.

%~d0
cd %~dp0
PUSHD %~dp0

::SETLOCAL ENABLEDELAYEDEXPANSION

:: if the user has set up a custom env call it
IF EXIST "%~dp0Env_Dev.bat" CALL %~dp0Env_Dev.bat

:: Initialize env
CALL %~dp0\Env_O3DE_Core.bat

:: add to the PATH here (this is global)
SET PATH=%PATH_O3DE_BIN%;%PATH_DCCSIG%;%PATH%

CALL %~dp0\Env_O3DE_Python.bat

:: add to the PYTHONPATH here (this is global)
SET PATH=%PATH_O3DE_PYTHON_INSTALL%;%O3DE_PYTHONHOME%;%PATH%

:: add all python related paths to PYTHONPATH for package imports
SET PYTHONPATH=%PATH_DCCSIG%;%PATH_DCCSI_PYTHON_LIB%;%PATH_O3DE_BIN%;%PYTHONPATH%

CALL %~dp0\Env_O3DE_Qt.bat

SET PATH=%QTFORPYTHON_PATH%;%QT_PLUGIN_PATH%;%PATH_O3DE_PYTHON_INSTALL%;%O3DE_PYTHONHOME%;%DCCSI_PY_IDE%;%PATH%
SET PYTHONPATH=%QTFORPYTHON_PATH%;%PATH_O3DE_TECHART_GEMS%;%PATH_DCCSIG%;%PATH_DCCSI_PYTHON_LIB%;%PATH_O3DE_BUILD%;%PYTHONPATH%

:: if the user has set up a custom env call it
IF EXIST "%~dp0Env_Dev.bat" CALL %~dp0Env_Dev.bat

echo.
echo _____________________________________________________________________
echo.
echo ~     Starting O3DE DCCsi python CMD
echo _____________________________________________________________________
echo.

echo.
echo     PATH = %PATH%
echo.
echo     PYTHONPATH = %PYTHONPATH%
echo.

:: Change to root dir
CD /D %PATH_O3DE_PROJECT%

:: Create command prompt with environment
CALL %windir%\system32\cmd.exe

ENDLOCAL

:: Return to starting directory
POPD

:END_OF_FILE
