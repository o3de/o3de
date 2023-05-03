@echo off
REM
REM Copyright (c) Contributors to the Open 3D Engine Project.
REM For complete copyright and license terms please see the LICENSE at the root of this distribution.
REM
REM SPDX-License-Identifier: Apache-2.0 OR MIT
REM
REM

:: Launches Maya bootstrapped with O3DE and DccScriptingInterface
:: Status: Prototype
:: Version: 0.0.1
:: Support: Maya 2022 and 2023 (python 3)
:: Readme.md:  https://github.com/o3de/o3de/tree/development/Gems/AtomLyIntegration/TechnicalArt/DccScriptingInterface/Tools/DCC/Maya
:: Notes:
:: - If you have a non-standard default install location for Maya, you may need  to do manual configuration
:: - Try overriding envars and paths in your Env_Dev.bat

:: Set up window
TITLE O3DE DCCsi Launch Maya
:: Use obvious color to prevent confusion (Grey with Yellow Text)
COLOR 8E

echo.
echo _____________________________________________________________________
echo.
echo ~    O3DE DCCsi Maya Launch Env ...
echo _____________________________________________________________________
echo.
echo ~    default envas for Maya env
echo.

:: Store current dir
%~d0
cd %~dp0
PUSHD %~dp0

SETLOCAL ENABLEDELAYEDEXPANSION

:: if the user has set up a custom env call it
IF EXIST "%~dp0Env_Dev.bat" CALL %~dp0Env_Dev.bat

:: Default Maya and Python version
IF "%MAYA_VERSION%"=="" (set MAYA_VERSION=2023)
IF "%DCCSI_PY_VERSION_MAJOR%"=="" (set DCCSI_PY_VERSION_MAJOR=3)
IF "%DCCSI_PY_VERSION_MINOR%"=="" (set DCCSI_PY_VERSION_MINOR=9)
IF "%DCCSI_PY_VERSION_RELEASE%"=="" (set DCCSI_PY_VERSION_RELEASE=7)

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

CALL %~dp0..\..\Dev\Windows\Env_DCC_Maya.bat

:: ide and debugger plug
set "DCCSI_PY_DEFAULT=%MAYA_BIN_PATH%\mayapy.exe"
echo     DCCSI_PY_DEFAULT = %DCCSI_PY_DEFAULT%

:: Some IDEs like Wing, may in some cases need acess directly to the exe to operate correctly
set "DCCSI_PY_IDE=%MAYA_BIN_PATH%\mayapy.exe"
echo     DCCSI_PY_IDE = %DCCSI_PY_IDE%

:: add to the PATH here (this is global)
SET PATH=%PATH_O3DE_BIN%;%PATH_DCCSIG%;%PATH%

SET PATH=%MAYA_BIN_PATH%;%DCCSI_PY_IDE%;%DCCSI_PY_DEFAULT%;%PATH%

:: add all python related paths to PYTHONPATH for package imports
set PYTHONPATH=%DCCSI_MAYA_SCRIPT_PATH%;%PATH_O3DE_TECHART_GEMS%;%PATH_DCCSIG%;%PATH_DCCSI_PYTHON_LIB%;%PATH_O3DE_BIN%;%PYTHONPATH%

:: if the user has set up a custom env call it
:: this ensures any user envars remain intact after initializing other envs
IF EXIST "%~dp0Env_Dev.bat" CALL %~dp0Env_Dev.bat


echo.
echo _____________________________________________________________________
echo.
echo ~    Launching O3DE DCCsi Maya (%MAYA_VERSION%) ...
echo ________________________________________________________________
echo.

echo     MAYA_VERSION = %MAYA_VERSION%
echo     DCCSI_PY_VERSION_MAJOR = %DCCSI_PY_VERSION_MAJOR%
echo     DCCSI_PY_VERSION_MINOR = %DCCSI_PY_VERSION_MINOR%
echo     DCCSI_PY_VERSION_RELEASE = %DCCSI_PY_VERSION_RELEASE%
echo     MAYA_LOCATION = %MAYA_LOCATION%
echo     MAYA_BIN_PATH = %MAYA_BIN_PATH%

echo.
echo     PATH = %PATH%
echo.
echo     PYTHONPATH = %PYTHONPATH%
echo.

:: Change to root dir
CD /D %PATH_O3DE_PROJECT%

:: Default to the right version of Maya if we can detect it... and launch
IF EXIST "%MAYA_BIN_PATH%\maya.exe" (
    start "" "%MAYA_BIN_PATH%\maya.exe" %*
) ELSE (
    Where maya.exe 2> NUL
    IF ERRORLEVEL 1 (
        echo Maya.exe could not be found
            pause
    ) ELSE (
        start "" Maya.exe %*
    )
)

::ENDLOCAL

:: Restore previous directory
POPD

:END_OF_FILE
