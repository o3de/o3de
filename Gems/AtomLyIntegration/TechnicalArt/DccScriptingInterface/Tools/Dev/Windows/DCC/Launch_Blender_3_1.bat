@echo off
REM 
REM Copyright (c) Contributors to the Open 3D Engine Project.
REM For complete copyright and license terms please see the LICENSE at the root of this distribution.
REM
REM SPDX-License-Identifier: Apache-2.0 OR MIT
REM
REM

:: Store current dir
%~d0
cd %~dp0
PUSHD %~dp0

SETLOCAL ENABLEDELAYEDEXPANSION

:: if the user has set up a custom env call it
IF EXIST "%~dp0..\Env_Dev.bat" CALL %~dp0..\Env_Dev.bat

:: Default Maya and Python version
set BLENDER_VERSION=3.1
set DCCSI_PY_VERSION_MAJOR=3
set DCCSI_PY_VERSION_MINOR=7
set DCCSI_PY_VERSION_RELEASE=7

CALL %~dp0..\Env_O3DE_Core.bat

:: add to the PATH here (this is global)
SET PATH=%PATH_O3DE_BIN%;%PATH_DCCSIG%;%PATH%

CALL %~dp0..\Env_DCC_Blender.bat

IF "%BLENDER_BIN_PATH%"=="" (set "BLENDER_BIN_PATH=%BLENDER_LOCATION%\bin")
echo     BLENDER_BIN_PATH = %BLENDER_BIN_PATH%

:: ide and debugger plug
set "DCCSI_PY_DEFAULT=%BLENDER_BIN_PATH%\python.exe"
echo     DCCSI_PY_DEFAULT = %DCCSI_PY_DEFAULT%

:: Some IDEs like Wing, may in some cases need acess directly to the exe to operate correctly
set "DCCSI_PY_IDE=%BLENDER_BIN_PATH%\blender.exe"
echo     DCCSI_PY_IDE = %DCCSI_PY_IDE%

SET PATH=%BLENDER_BIN_PATH%;%DCCSI_PY_IDE%;%DCCSI_PY_DEFAULT%;%PATH%

:: the next line sets up too much, I beleive is causing a maya boot failure
::CALL %~dp0..\Env_O3DE_Python.bat

:: shared location for 64bit python 3.7 DEV location
:: this defines a DCCsi sandbox for lib site-packages by version
:: <O3DE>\Gems\AtomLyIntegration\TechnicalArt\DccScriptingInterface\3rdParty\Python\Lib
set "PATH_DCCSI_PYTHON=%PATH_DCCSIG%\3rdParty\Python"
echo     PATH_DCCSI_PYTHON = %PATH_DCCSI_PYTHON%

:: add access to a Lib location that matches the py version (example: 3.7.x)
:: switch this for other python versions like maya (2.7.x)
IF "%PATH_DCCSI_PYTHON_LIB%"=="" (set "PATH_DCCSI_PYTHON_LIB=%PATH_DCCSI_PYTHON%\Lib\%DCCSI_PY_VERSION_MAJOR%.x\%DCCSI_PY_VERSION_MAJOR%.%DCCSI_PY_VERSION_MINOR%.x\site-packages")
echo     PATH_DCCSI_PYTHON_LIB = %PATH_DCCSI_PYTHON_LIB%

:: add all python related paths to PYTHONPATH for package imports
::set PYTHONPATH=%DCCSI_BLENDER_SCRIPTS%;%DCCSI_BLENDER_SCRIPTS%;%PYTHONPATH%
::echo     PYTHONPATH = %PYTHONPATH%

:: add to the PATH
SET PATH=%DCCSI_BLENDER_LOCATION%;%DCCSI_BLENDER_PY_BIN%;%PATH%

:: add all python related paths to PYTHONPATH for package imports
set PYTHONPATH=%DCCSI_BLENDER_SCRIPT_PATH%;%PATH_DCCSIG%;%PATH_DCCSI_PYTHON_LIB%;%PYTHONPATH%

:: if the user has set up a custom env call it
IF EXIST "%~dp0..\Env_Dev.bat" CALL %~dp0..\Env_Dev.bat

echo.
echo _____________________________________________________________________
echo.
echo Launching Maya %BLENDER_VERSION% for O3DE DCCsi...
echo _____________________________________________________________________
echo.

echo     BLENDER_VERSION = %BLENDER_VERSION%
echo     DCCSI_PY_VERSION_MAJOR = %DCCSI_PY_VERSION_MAJOR%
echo     DCCSI_PY_VERSION_MINOR = %DCCSI_PY_VERSION_MINOR%
echo     DCCSI_PY_VERSION_RELEASE = %DCCSI_PY_VERSION_RELEASE%
echo     BLENDER_LOCATION = %BLENDER_LOCATION%
echo     BLENDER_BIN_PATH = %BLENDER_BIN_PATH%

echo.
echo     PATH = %PATH%
echo.
echo     PYTHONPATH = %PYTHONPATH%
echo.

:: Change to root dir
CD /D %PATH_O3DE_PROJECT%

:: Default to the right version of Maya if we can detect it... and launch
IF EXIST "%BLENDER_BIN_PATH%\blender.exe" (
    start "" "%BLENDER_BIN_PATH%\blender.exe" %*
) ELSE (
    Where blender.exe 2> NUL
    IF ERRORLEVEL 1 (
        echo blender.exe could not be found
            pause
    ) ELSE (
        start "" blender.exe %*
    )
)

::ENDLOCAL

:: Restore previous directory
POPD

:END_OF_FILE