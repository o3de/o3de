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

:: Launches maya wityh a bunch of local hooks for AtomTressFX (Hair) plugins
:: ToDo: move all of this to a .json data driven boostrapping system

@echo off

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
set MAYA_PROJECT=%LY_PROJECT_PATH%
echo     MAYA_PROJECT = %LY_PROJECT_PATH%

:: DX11 Viewport
Set MAYA_VP2_DEVICE_OVERRIDE = VirtualDeviceDx11

:: add plug-in path to AtomTressFX
set TRESSFX_PLUG_IN_PATH=%LY_DEV%\Gems\AtomTressFX\Tools\Maya
:: also attached to maya's built-it env var
set MAYA_PLUG_IN_PATH=%TRESSFX_PLUG_IN_PATH%;%MAYA_PLUG_IN_PATH%
echo     MAYA_PLUG_IN_PATH = %MAYA_PLUG_IN_PATH%

:: configure local xgen data so we can store it with test data
:: https://knowledge.autodesk.com/support/maya/learn-explore/caas/CloudHelp/cloudhelp/2020/ENU/Maya-CharEffEnvBuild/files/GUID-6ED517C7-7346-4A6E-A9CF-37D2B8511C36-htm.html
Set XGEN_CONFIG_PATH=%LY_PROJECT_PATH%\AssetData
echo     XGEN_CONFIG_PATH = %XGEN_CONFIG_PATH%

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