:: Launches maya wityh a bunch of local hooks for Lumberyard
:: ToDo: move all of this to a .json data driven boostrapping system

@echo off

REM 
REM All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
REM its licensors.
REM
REM For complete copyright and license terms please see the LICENSE at the root of this
REM distribution (the "License"). All use of this software is governed by the License,
REM or, if provided, by the license below or the license accompanying this file. Do not
REM remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
REM WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
REM

echo ________________________________
echo ~ calling DccScriptingInterface_Env.bat

%~d0
cd %~dp0
PUSHD %~dp0

:: Keep changes local
SETLOCAL enableDelayedExpansion

:: PY version Major
set DCCSI_PY_VERSION_MAJOR=2
echo     DCCSI_PY_VERSION_MAJOR = %DCCSI_PY_VERSION_MAJOR%

:: PY version Major
set DCCSI_PY_VERSION_MINOR=7
echo     DCCSI_PY_VERSION_MINOR = %DCCSI_PY_VERSION_MINOR%

:: Default Maya Version
set MAYA_VERSION=2020
echo     MAYA_VERSION = %MAYA_VERSION%

:: if a local customEnv.bat exists, run it
IF EXIST "%~dp0Env.bat" CALL %~dp0Env.bat

echo ________________________________
echo Launching Maya %MAYA_VERSION% for LY DCCsi...

::set MAYA_PATH="D:\Program Files\Autodesk\Maya2019"
echo     MAYA_PATH = %MAYA_PATH%

:::: Set Maya native project acess to this project
::set MAYA_PROJECT=%LY_PROJECT%
::echo     MAYA_PROJECT = %MAYA_PROJECT%

:: DX11 Viewport
Set MAYA_VP2_DEVICE_OVERRIDE=VirtualDeviceDx11

:: Default to the right version of Maya if we can detect it... and launch
IF EXIST "C:\Program Files\Autodesk\Maya%MAYA_VERSION%\bin\maya.exe" (
   start "" "C:\Program Files\Autodesk\Maya%MAYA_VERSION%\bin\maya.exe" %*
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