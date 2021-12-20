@echo off

REM 
REM Copyright (c) Contributors to the Open 3D Engine Project.
REM For complete copyright and license terms please see the LICENSE at the root of this distribution.
REM
REM SPDX-License-Identifier: Apache-2.0 OR MIT
REM
REM

%~d0
cd %~dp0
PUSHD %~dp0

echo ________________________________
echo ~ calling PROJ_Env.bat

:: Keep changes local
SETLOCAL enableDelayedExpansion

:: PY version Major
IF "%DCCSI_PY_VERSION_MAJOR%"=="" (set DCCSI_PY_VERSION_MAJOR=2)
echo     DCCSI_PY_VERSION_MAJOR = %DCCSI_PY_VERSION_MAJOR%

:: PY version Major
IF "%DCCSI_PY_VERSION_MINOR%"=="" (set DCCSI_PY_VERSION_MINOR=7)
echo     DCCSI_PY_VERSION_MINOR = %DCCSI_PY_VERSION_MINOR%

:: Maya Version
IF "%DCCSI_MAYA_VERSION%"=="" (set DCCSI_MAYA_VERSION=2020)
echo     DCCSI_MAYA_VERSION = %DCCSI_MAYA_VERSION%

:: if a local customEnv.bat exists, run it
IF EXIST "%~dp0Project_Env.bat" CALL %~dp0Project_Env.bat

echo ________________________________
echo Launching Maya %DCCSI_MAYA_VERSION% for O3DE: %O3DE_PROJECT%...

:::: Set Maya native project acess to this project
::set MAYA_PROJECT=%LY_PROJECT%
::echo     MAYA_PROJECT = %MAYA_PROJECT%

:: DX11 Viewport
Set MAYA_VP2_DEVICE_OVERRIDE = VirtualDeviceDx11

:: Default to the right version of Maya if we can detect it... and launch
echo     MAYA_BIN_PATH = %MAYA_BIN_PATH%

IF EXIST "%MAYA_BIN_PATH%\Maya.exe" (
   start "" "%MAYA_BIN_PATH%\Maya.exe" %*
) ELSE (
   Where maya.exe 2> NUL
   IF ERRORLEVEL 1 (
      echo Maya.exe could not be found
      pause
   ) ELSE (
      start "" Maya.exe %*
   )
)

:: Return to starting directory
POPD

:END_OF_FILE

exit /b 0
