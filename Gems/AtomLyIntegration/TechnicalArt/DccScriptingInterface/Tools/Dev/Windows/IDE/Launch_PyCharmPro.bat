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

echo.
echo _____________________________________________________________________
echo.
echo ~    O3DE DCCsi Env PyCharm Launcher ...
echo _____________________________________________________________________
echo.

:: Store current dir
%~d0
cd %~dp0
PUSHD %~dp0

::SETLOCAL ENABLEDELAYEDEXPANSION

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
CALL %~dp0..\Env_O3DE_Core.bat

:: add to the PATH here (this is global)
SET PATH=%PATH_O3DE_BIN%;%PATH_DCCSIG%;%PATH%

:: Initialize env
CALL %~dp0\Env_O3DE_Python.bat

:: add to the PATH here (this is global)
SET PATH=%PATH_O3DE_PYTHON_INSTALL%;%O3DE_PYTHONHOME%;%DCCSI_PY_IDE%;%PATH%

:: add all python related paths to PYTHONPATH for package imports
SET PYTHONPATH=%PATH_DCCSIG%;%PATH_DCCSI_PYTHON_LIB%;%PATH_O3DE_BUILD%;%PYTHONPATH%

:: Initialize env
CALL %~dp0\Env_O3DE_Qt.bat

:: add to the PATH
SET PATH=%QTFORPYTHON_PATH%;%PATH%
SET PYTHONPATH=%QTFORPYTHON_PATH%;%PYTHONPATH%

:: add to the PATH
SET PATH=%QT_PLUGIN_PATH%;%PATH%
SET PYTHONPATH=%QT_PLUGIN_PATH%;%PYTHONPATH%

SET PATH=%PATH_O3DE_BIN%;%PATH%

:: Initialize env
CALL %~dp0\..\Env_DCC_Maya.bat
CALL %~dp0\..\Env_DCC_Blender.bat
CALL %~dp0\..\Env_DCC_Substance.bat
CALL %~dp0\..\Env_IDE_PyCharm.bat

:: add to the PATH here (this is global)
SET PATH=%PYCHARM_HOME%;%PATH%

echo.
echo _____________________________________________________________________
echo.
echo ~ Launching DCCsi Project in PyCharm %PYCHARM_VERSION_YEAR%.%PYCHARM_VERSION_MAJOR% ...
echo _____________________________________________________________________
echo.

echo.
echo     PATH = %PATH%
echo.
echo     PYTHONPATH = %PYTHONPATH%
echo.

:: Change to root dir
CD /D %PATH_O3DE_PROJECT%

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

ENDLOCAL

:: Return to starting directory
POPD

:END_OF_FILE
