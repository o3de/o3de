@echo off

REM 
REM Copyright (c) Contributors to the Open 3D Engine Project.
REM For complete copyright and license terms please see the LICENSE at the root of this distribution.
REM
REM SPDX-License-Identifier: Apache-2.0 OR MIT
REM
REM

:: Launches VScode and the DccScriptingInterface Project Files

:: Set up window
TITLE O3DE DCC Scripting Interface VScode
:: Use obvious color to prevent confusion (Grey with Yellow Text)
COLOR 8E

echo.
echo _____________________________________________________________________
echo.
echo ~    Setting up O3DE DCCsi VScode Dev Env...
echo _____________________________________________________________________
echo.

:: Store current dir
%~d0
cd %~dp0
PUSHD %~dp0

:: Constant Vars (Global)
:: global debug (propogates)
IF "%DCCSI_GDEBUG%"=="" (set DCCSI_GDEBUG=False)
echo     DCCSI_GDEBUG = %DCCSI_GDEBUG%
:: initiates debugger connection
IF "%DCCSI_DEV_MODE%"=="" (set DCCSI_DEV_MODE=False)
echo     DCCSI_DEV_MODE = %DCCSI_DEV_MODE%
:: sets debugger, options: WING, PYCHARM
IF "%DCCSI_GDEBUGGER%"=="" (set DCCSI_GDEBUGGER=WING)
echo     DCCSI_GDEBUGGER = %DCCSI_GDEBUGGER%
:: Default level logger will handle
:: CRITICAL:50
:: ERROR:40
:: WARNING:30
:: INFO:20
:: DEBUG:10
:: NOTSET:0
IF "%DCCSI_LOGLEVEL%"=="" (set DCCSI_LOGLEVEL=20)
echo     DCCSI_LOGLEVEL = %DCCSI_LOGLEVEL%

:: if the user has set up a custom env call it
IF EXIST "%~dp0..\Env_Dev.bat" CALL %~dp0..\Env_Dev.bat

:: Initialize env
CALL %~dp0\..\Env_O3DE_Core.bat

:: add to the PATH here (this is global)
SET PATH=%PATH_O3DE_BIN%;%PATH_DCCSIG%;%PATH%

CALL %~dp0\..\Env_O3DE_Python.bat

:: add to the PYTHONPATH here (this is global)
SET PATH=%PATH_O3DE_PYTHON_INSTALL%;%O3DE_PYTHONHOME%;%PATH%

:: add all python related paths to PYTHONPATH for package imports
set PYTHONPATH=%PATH_DCCSIG%;%PATH_DCCSI_PYTHON_LIB%;%PATH_O3DE_BIN%;%PYTHONPATH%

CALL %~dp0\..\Env_O3DE_Qt.bat

SET PATH=%PATH_O3DE_PYTHON_INSTALL%;%O3DE_PYTHONHOME%;%DCCSI_PY_IDE%;%PATH%
SET PYTHONPATH=%PATH_DCCSIG%;%PATH_DCCSI_PYTHON_LIB%;%PATH_O3DE_BUILD%;%PYTHONPATH%

CALL %~dp0\..\Env_IDE_VScode.bat

SET PATH=%VSCODEHOME%;%PATH%

:: if the user has set up a custom env call it
IF EXIST "%~dp0..\Env_Dev.bat" CALL %~dp0..\Env_Dev.bat

echo.
echo _____________________________________________________________________
echo.
echo ~ Launching DCCsi Project in VScode
echo _____________________________________________________________________
echo.

echo.
echo     PATH = %PATH%
echo.
echo     PYTHONPATH = %PYTHONPATH%
echo.

REM "C:\Program Files\Microsoft VS Code\Code.exe"

IF EXIST "%ProgramFiles%\Microsoft VS Code\Code.exe" (
    start "" "%ProgramFiles%\Microsoft VS Code\Code.exe" "%VSCODE_WRKSPC%"
) ELSE (
    Where Code.exe 2> NUL
    IF ERRORLEVEL 1 (
        echo Code.exe could not be found
            pause
    ) ELSE (
        start "" code.exe "%VSCODE_WRKSPC%"
    )
)

:: Return to starting directory
POPD

:END_OF_FILE
