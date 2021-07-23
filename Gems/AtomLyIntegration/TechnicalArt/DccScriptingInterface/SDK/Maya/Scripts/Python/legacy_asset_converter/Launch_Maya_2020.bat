:: coding:utf-8
:: !/usr/bin/python
::
:: Copyright (c) Contributors to the Open 3D Engine Project.
:: For complete copyright and license terms please see the LICENSE at the root of this distribution.
::
:: SPDX-License-Identifier: Apache-2.0 OR MIT
::
::

@echo off
:: Launches maya with a bunch of local hooks for Lumberyard
:: ToDo: move all of this to a .json data driven boostrapping system

%~d0
cd %~dp0
PUSHD %~dp0

echo ________________________________
echo ~ calling PROJ_Env.bat

:: Keep changes local
SETLOCAL enableDelayedExpansion

:: PY version Major
set DCCSI_PY_VERSION_MAJOR=2
echo     DCCSI_PY_VERSION_MAJOR = %DCCSI_PY_VERSION_MAJOR%

:: PY version Major
set DCCSI_PY_VERSION_MINOR=7
echo     DCCSI_PY_VERSION_MINOR = %DCCSI_PY_VERSION_MINOR%

:: Maya Version
set MAYA_VERSION=2020
echo     MAYA_VERSION = %MAYA_VERSION%

:: if a local customEnv.bat exists, run it
IF EXIST "%~dp0Project_Env.bat" CALL %~dp0Project_Env.bat

echo ________________________________
echo Launching Maya %MAYA_VERSION% for Lumberyard...

:::: Set Maya native project acess to this project
::set MAYA_PROJECT=%LY_PROJECT%
::echo     MAYA_PROJECT = %MAYA_PROJECT%

:: DX11 Viewport
Set MAYA_VP2_DEVICE_OVERRIDE = VirtualDeviceDx11

:: Default to the right version of Maya if we can detect it... and launch
IF EXIST "%MAYA_LOCATION%\bin\Maya.exe" (
   start "" "%MAYA_LOCATION%\bin\Maya.exe" %*
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
