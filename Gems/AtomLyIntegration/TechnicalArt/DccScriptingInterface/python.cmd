@ECHO OFF
REM
REM Copyright (c) Contributors to the Open 3D Engine Project.
REM For complete copyright and license terms please see the LICENSE at the root of this distribution.
REM
REM SPDX-License-Identifier: Apache-2.0 OR MIT
REM
REM

REM This script provides a single entry point that you can trust is present.
REM Depending on this entry point instead of trying to find a python.exe
REM In a subfolder allows you to keep working if the version of python changes or
REM other environmental requirements change.
REM When the project switches to a new version of Python, this file will be updated.

:: Set up window
TITLE O3DE DCC Scripting Interface Python.cmd
:: Use obvious color to prevent confusion (Grey with Yellow Text)
COLOR 8E

echo.
echo _____________________________________________________________________
echo.
echo ~    O3DE DCCsi Python.cmd ...
echo _____________________________________________________________________
echo.

SETLOCAL
SET CMD_DIR=%~dp0
SET CMD_DIR=%CMD_DIR:~0,-1%

IF "%PATH_DCCSIG%"=="" (set "PATH_DCCSIG=%CMD_DIR%")
echo     PATH_DCCSIG = %PATH_DCCSIG%

:: Change to DCCsi root dir
CD /D %PATH_DCCSIG%

IF "%PATH_O3DE_PROJECT%"=="" (set "PATH_O3DE_PROJECT=%PATH_DCCSIG%")

:: This maps up to the o3de root folder
IF "%O3DE_REL_PATH%"=="" (set "O3DE_REL_PATH=..\..\..\..")

:: Change to root o3de folder
CD /d %O3DE_REL_PATH%
IF "%O3DE_DEV%"=="" (set "O3DE_DEV=%CD%")
:: Restore original directory
popd

SET O3DE_PYTHON=%O3DE_DEV%\python
echo     O3DE_PYTHON = %O3DE_PYTHON%

:: O3DE Technical Art Gems Location
IF "%PATH_O3DE_TECHART_GEMS%"=="" (set "PATH_O3DE_TECHART_GEMS=%O3DE_DEV%\Gems\AtomLyIntegration\TechnicalArt")
echo     PATH_O3DE_TECHART_GEMS = %PATH_O3DE_TECHART_GEMS%

:: O3DE DccScriptingInterface Gem location
IF "%PATH_DCCSIG%"=="" (set "DccScriptingInterface")
echo     PATH_DCCSIG = %PATH_DCCSIG%

SET PATH=%O3DE_DEV%;%PATH_O3DE_TECHART_GEMS%;%PATH_DCCSIG%;%PATH%
SET PYTHONPATH=%O3DE_DEV%;%PATH_O3DE_TECHART_GEMS%;%PATH_DCCSIG%;%PYTHONPATH%

:: get the o3de python home
FOR /F "tokens=* USEBACKQ" %%F IN (`%O3DE_PYTHON%\python.cmd %O3DE_PYTHON%\get_python_path.py`) DO (SET O3DE_PYTHONHOME=%%F)

IF EXIST "%O3DE_PYTHONHOME%" GOTO PYTHONHOME_EXISTS

ECHO Python not found in %O3DE_PYTHON%
ECHO Try running %O3DE_PYTHON%\get_python.bat first.
exit /B 1

:PYTHONHOME_EXISTS

SET PYTHON=%O3DE_PYTHONHOME%\python.exe
echo     PYTHON = %PYTHON%

SET PYTHON_ARGS=%*

IF [%1] EQU [debug] (
    SET PYTHON=%O3DE_PYTHONHOME%\python_d.exe
    SET PYTHON_ARGS=%PYTHON_ARGS:~6%
)

IF EXIST "%PYTHON%" GOTO PYTHON_EXISTS

ECHO Could not find python executable at %PYTHON%
exit /B 1

:PYTHON_EXISTS

:: Change to root dir
CD /D %PATH_DCCSIG%

echo _____________________________________________________________________
echo.

SET PYTHONNOUSERSITE=1
"%PYTHON%" %PYTHON_ARGS%
exit /B %ERRORLEVEL%
