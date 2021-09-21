@echo off
REM 
REM Copyright (c) Contributors to the Open 3D Engine Project.
REM For complete copyright and license terms please see the LICENSE at the root of this distribution.
REM
REM SPDX-License-Identifier: Apache-2.0 OR MIT
REM
REM

:: Set up window
TITLE O3DE DCCsi GEM PyCharm
:: Use obvious color to prevent confusion (Grey with Yellow Text)
COLOR 8E

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

:: Initialize env
CALL %~dp0\Env_Core.bat
CALL %~dp0\Env_Python.bat
CALL %~dp0\Env_PyCharm.bat
CALL %~dp0\Env_Maya.bat

set DCCSI_PY_DEFAULT=%DCCSI_PY_MAYA%

:: add prefered python to the PATH
set PATH=%DCCSI_PY_DEFAULT%;%PATH%

echo.
echo _____________________________________________________________________
echo.
echo ~ Launching DCCsi Project in PyCharm %PYCHARM_VER_YEAR%.%PYCHARM_VER_MAJOR%.%PYCHARM_VER_MINOR% ...
echo ~ MayaPy.exe (default python interpreter)
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
set DCCSI_PY_DEFAULT=%DCCSI_PY_MAYA%
echo     DCCSI_PY_DEFAULT = %DCCSI_PY_DEFAULT%

:: if the user has set up a custom env call it
IF EXIST "%~dp0Env_Dev.bat" CALL %~dp0Env_Dev.bat

echo.

:: Change to root dir
CD /D %O3DE_PROJECT_PATH%

IF EXIST "%PYCHARM_HOME%\bin\pycharm64.exe" (
    start "" "%PYCHARM_HOME%\bin\pycharm64.exe" "%PYCHARM_PROJ%"
) ELSE (
    Where pycharm64.exe 2> NUL
    IF ERRORLEVEL 1 (
        echo pycharm64.exe could not be found
            pause
    ) ELSE (
        start "" pycharm64.exe "%PYCHARM_PROJ%"
    )
)

:: Return to starting directory
POPD

:END_OF_FILE
