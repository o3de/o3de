@echo off
REM
REM Copyright (c) Contributors to the Open 3D Engine Project.
REM For complete copyright and license terms please see the LICENSE at the root of this distribution.
REM
REM SPDX-License-Identifier: Apache-2.0 OR MIT
REM
REM

:: Sets up extended DCCsi environment for Adobe Substance Designer

:: Set up window
TITLE O3DE DCCsi Autodesk Substance Designer Environment
:: Use obvious color to prevent confusion (Grey with Yellow Text)
COLOR 8E

:: Skip initialization if already completed
IF "%DCCSI_ENV_SUBSTANCE_INIT%"=="1" GOTO :END_OF_FILE

:: Store current dir
%~d0
cd %~dp0
PUSHD %~dp0

:: maintenance: latest version of Adobe CC Substance Designer, June-23-2022
:: print(sys.version): 3.9.9 (tags/v3.9.9:ccb0e6a, Nov 15 2021, 18:08:50) [MSC v.1929 64 bit (AMD64)
IF "%DCCSI_PY_VERSION_MAJOR%"=="" (set DCCSI_PY_VERSION_MAJOR=3)
IF "%DCCSI_PY_VERSION_MINOR%"=="" (set DCCSI_PY_VERSION_MINOR=9)
IF "%DCCSI_PY_VERSION_RELEASE%"=="" (set DCCSI_PY_VERSION_RELEASE=9)

:: If you want to override the python version and/or Substance paths
:: create a Env_Dev.bat file and set envars you would like to override

:: example file: "<DccScriptingInterface>\Tools\Dev\Windows\Env_Dev.bat.example"

:: There are two locations you can do this
:: DccScriptingInterface\Tools\DCC\Substance\Env_Dev.bat
:: This will only fire off with: "<DccScriptingInterface>\Tools\DCC\Substance\windows_launch_substabce_designer.bat"

:: <DccScriptingInterface>\Tools\Dev\Windows\Env_Dev.bat
:: This will execute anytime the core is called: "<DccScriptingInterface>\Tools\Dev\Windows\Env_DCC_Substance.bat"

:: This gives us core access to O3DE but puts nothing on the PATH or PYTHONPATH
CALL %~dp0\Env_O3DE_Core.bat
CALL %~dp0\Env_O3DE_Python.bat
:: We must be careful not to muck with Substance built-in python or Qt
:: Incorrect configuration will cause conflicts, likely a app boot failure!

echo.
echo _____________________________________________________________________
echo.
echo ~    O3DE DCCsi Adobe Substance Environment ...
echo _____________________________________________________________________
echo.

: Substance Designer
IF "%DCCSI_SUBSTANCE_PROJECT%"=="" (set DCCSI_SUBSTANCE_PROJECT=%PATH_O3DE_PROJECT%)
echo     DCCSI_SUBSTANCE_PROJECT = %DCCSI_SUBSTANCE_PROJECT%

:: O3DE DCCsi Substance location
IF "%DCCSI_TOOLS_SUBSTANCE%"=="" (set "DCCSI_TOOLS_SUBSTANCE=%PATH_DCCSI_TOOLS%\DCC\Substance")
echo     DCCSI_TOOLS_SUBSTANCE = %DCCSI_TOOLS_SUBSTANCE%

IF "%DCCSI_SUBSTANCE_RESOURCES%"=="" (set "DCCSI_SUBSTANCE_RESOURCES=%DCCSI_TOOLS_SUBSTANCE%\Resources")
echo     DCCSI_SUBSTANCE_RESOURCES = %DCCSI_SUBSTANCE_RESOURCES%

:: Path to .exe, "C:\Program Files\Adobe\Adobe Substance 3D Designer\Adobe Substance 3D Designer.exe"
IF "%DCCSI_SUBSTANCE_LOCATION%"=="" (set "DCCSI_SUBSTANCE_LOCATION=%ProgramFiles%\Adobe\Adobe Substance 3D Designer")
echo     DCCSI_SUBSTANCE_LOCATION = %DCCSI_SUBSTANCE_LOCATION%

IF "%DCCSI_SUBSTANCE_EXE%"=="" (set "DCCSI_SUBSTANCE_EXE=%DCCSI_SUBSTANCE_LOCATION%\Adobe Substance 3D Designer.exe")
echo     DCCSI_SUBSTANCE_EXE = %DCCSI_SUBSTANCE_EXE%

:: substance python location
IF "%DCCSI_SUBSTANCE_PYTHON%"=="" (set "DCCSI_SUBSTANCE_PYTHON=%DCCSI_SUBSTANCE_LOCATION%\plugins\pythonsdk")
echo     DCCSI_SUBSTANCE_PYTHON = %DCCSI_SUBSTANCE_PYTHON%

:: substance python exe
IF "%DCCSI_SUBSTANCE_PY_EXE%"=="" (set "DCCSI_SUBSTANCE_PY_EXE=%DCCSI_SUBSTANCE_PYTHON%\python.exe")
echo     DCCSI_SUBSTANCE_PY_EXE = %DCCSI_SUBSTANCE_PY_EXE%

:: set as a interpreter hook for wing ide
set "DCCSI_PY_SUBSTANCE=%DCCSI_SUBSTANCE_PY_EXE%"
echo     DCCSI_PY_SUBSTANCE = %DCCSI_PY_SUBSTANCE%

:: substance has a callback system, we don't use it yet
IF "%DCCSI_SUBSTANCE_SET_CALLBACKS%"=="" (set DCCSI_SUBSTANCE_SET_CALLBACKS=false)
echo     DCCSI_SUBSTANCE_SET_CALLBACKS = %DCCSI_SUBSTANCE_SET_CALLBACKS%

:: python scripts
IF "%DCCSI_SUBSTANCE_SCRIPTS%"=="" (set "DCCSI_SUBSTANCE_SCRIPTS=%DCCSI_SUBSTANCE_RESOURCES%\Python")
echo     DCCSI_SUBSTANCE_SCRIPTS = %DCCSI_SUBSTANCE_SCRIPTS%

IF "%DCCSI_SUBSTANCE_CFG_SLUG%"=="" (set "DCCSI_SUBSTANCE_CFG_SLUG=o3de_dccsi.sbscfg")
echo     DCCSI_SUBSTANCE_CFG_SLUG = %DCCSI_SUBSTANCE_CFG_SLUG%

:: o3de dccsi substance config
:: https://substance3d.adobe.com/documentation/sddoc/project-settings-188974384.html
IF "%DCCSI_SUBSTANCE_CFG%"=="" (set "DCCSI_SUBSTANCE_CFG=%DCCSI_TOOLS_SUBSTANCE%\%DCCSI_SUBSTANCE_CFG_SLUG%")
echo     DCCSI_SUBSTANCE_CFG = %DCCSI_SUBSTANCE_CFG%

::ENDLOCAL

:: Set flag so we don't initialize dccsi environment twice
SET DCCSI_ENV_SUBSTANCE_INIT=1
GOTO END_OF_FILE

:: Return to starting directory
POPD

:END_OF_FILE
