@echo off
REM
REM Copyright (c) Contributors to the Open 3D Engine Project.
REM For complete copyright and license terms please see the LICENSE at the root of this distribution.
REM
REM SPDX-License-Identifier: Apache-2.0 OR MIT
REM
REM

:: Launches Wing IDE and the O3DE DccScriptingInterface Project Files
:: Status: Prototype
:: Version: 0.0.1
:: Support: Wing Pro 8+
:: Readme.md:  https://github.com/o3de/o3de/tree/development/Gems/AtomLyIntegration/TechnicalArt/DccScriptingInterface/Tools/IDE/WingIDE/readme.md
:: Notes:
:: - Wing 7.x was previously supported, but it was python2.7 based and we are deprecating support for py2.7
:: - py2.7 deprecation includes apps that are pre-py3
:: - Previous versions may still work, however you will need to configure the env yourself
:: - Try overriding envars and paths in your Env_Dev.bat

:: Set up window
TITLE O3DE DCCsi Launch WingIDE
:: Use obvious color to prevent confusion (Grey with Yellow Text)
COLOR 8E

echo.
echo _____________________________________________________________________
echo.
echo ~    O3DE DCCsi Wing Pro Launch Env ...
echo _____________________________________________________________________
echo.
echo ~    default envas for wing pro env
echo.

:: Store current dir
%~d0
cd %~dp0
PUSHD %~dp0

SETLOCAL ENABLEDELAYEDEXPANSION

:: if the user has set up a custom env call it
IF EXIST "%~dp0Env_Dev.bat" CALL %~dp0Env_Dev.bat

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

CALL %~dp0..\..\Dev\Windows\Env_O3DE_Core.bat

:: add to the PATH here (this is global)
SET PATH=%PATH_O3DE_BIN%;%PATH_O3DE_TECHART_GEMS%;%PATH_DCCSIG%;%PATH%

:: Initialize env
CALL %~dp0..\..\Dev\Windows\Env_O3DE_Python.bat

:: add to the PATH here (this is global)
SET PATH=%O3DE_PYTHONHOME%;%PATH_O3DE_PYTHON_INSTALL%;%DCCSI_PY_IDE%;%PATH%

:: add all python related paths to PYTHONPATH for package imports
SET PYTHONPATH=%PATH_O3DE_TECHART_GEMS%;%PATH_DCCSIG%;%PATH_DCCSI_PYTHON_LIB%;%PATH_O3DE_BUILD%

:: This can now only be added late, in the launcher
:: it conflicts with other Qt apps like Wing Pro 8+
REM :: Initialize env
REM CALL %~dp0..\..\Dev\Windows\Env_O3DE_Qt.bat

REM :: add to the PATH
REM SET PATH=%QTFORPYTHON_PATH%;%PATH%
REM SET PYTHONPATH=%QTFORPYTHON_PATH%;%PYTHONPATH%

REM :: add to the PATH
REM :: SET PATH=%QT_PLUGIN_PATH%;%PATH%
REM SET PYTHONPATH=%QT_PLUGIN_PATH%;%PYTHONPATH%

SET PATH=%PATH_O3DE_BIN%;%PATH%

:: Initialize env
CALL %~dp0..\..\Dev\Windows\Env_DCC_Maya.bat
SET PATH=%DCCSI_PY_MAYA%;%PATH%

CALL %~dp0..\..\Dev\Windows\Env_DCC_Blender.bat
SET PATH=%DCCSI_BLENDER_PY_EXE%;%PATH%

CALL %~dp0..\..\Dev\Windows\Env_DCC_Substance.bat
SET PATH=%DCCSI_SUBSTANCE_PY_EXE%;%PATH%

CALL %~dp0..\..\Dev\Windows\Env_IDE_Wing.bat
SET PATH=%WINGHOME%;%PATH%

:: if the user has set up a custom env call it
:: this ensures any user envars remain intact after initializing other envs
IF EXIST "%~dp0Env_Dev.bat" CALL %~dp0Env_Dev.bat

echo.
echo _____________________________________________________________________
echo.
echo ~ WingIDE Version %DCCSI_WING_VERSION_MAJOR%
echo ~ Launching O3DE project: %O3DE_PROJECT% 
echo ~ in Wing Pro %DCCSI_WING_VERSION_MAJOR% ...
echo _____________________________________________________________________
echo.

echo.
echo     PATH = %PATH%
echo.
echo     PYTHONPATH = %PYTHONPATH%
echo.

:: Change to root dir
CD /D %PATH_O3DE_PROJECT%

pause

IF EXIST "%WINGHOME%\bin\wing.exe" (
    start "" "%WINGHOME%\bin\wing.exe" "%WING_PROJ%"
) ELSE (
    Where wing.exe 2> NUL
    IF ERRORLEVEL 1 (
        echo wing.exe could not be found
            pause
    ) ELSE (
        start "" wing.exe "%WING_PROJ%"
    )
)

::ENDLOCAL

:: Return to starting directory
POPD

:END_OF_FILE