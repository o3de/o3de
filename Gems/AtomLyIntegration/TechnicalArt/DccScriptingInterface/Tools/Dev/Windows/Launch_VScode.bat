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
IF "%DCCSI_GDEBUG%"=="" (set DCCSI_GDEBUG=True)
echo     DCCSI_GDEBUG = %DCCSI_GDEBUG%
:: initiates debugger connection
IF "%DCCSI_DEV_MODE%"=="" (set DCCSI_DEV_MODE=True)
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
IF "%DCCSI_LOGLEVEL%"=="" (set DCCSI_LOGLEVEL=10)
echo     DCCSI_LOGLEVEL = %DCCSI_LOGLEVEL%

:: Initialize envCALL %~dp0\Env_Core.bat
CALL %~dp0\Env_Python.bat
CALL %~dp0\Env_Qt.bat
CALL %~dp0\Env_Maya.bat
CALL %~dp0\Env_Substance.bat
CALL %~dp0\Env_VScode.bat

echo.
echo _____________________________________________________________________
echo.
echo ~ Launching DCCsi Project in VScode
echo _____________________________________________________________________
echo.

echo     O3DE_DEV = %O3DE_DEV%

:: shared location for default O3DE python location
set O3DE_PYTHON_INSTALL=%O3DE_DEV%\Python
echo     O3DE_PYTHON_INSTALL = %O3DE_PYTHON_INSTALL%

:: Wing and other IDEs probably prefer access directly to the python.exe
set DCCSI_PY_IDE = %O3DE_PYTHON_INSTALL%\runtime\python-3.7.10-rev2-windows\python
echo     DCCSI_PY_IDE = %DCCSI_PY_IDE%

:: ide and debugger plug
set DCCSI_PY_BASE=%DCCSI_PY_IDE%\python.exe
echo     DCCSI_PY_BASE = %DCCSI_PY_BASE%

:: ide and debugger plug
set DCCSI_PY_DEFAULT=%DCCSI_PY_BASE%
echo     DCCSI_PY_DEFAULT = %DCCSI_PY_DEFAULT%

:: if the user has set up a custom env call it
IF EXIST "%~dp0Env_Dev.bat" CALL %~dp0Env_Dev.bat

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
