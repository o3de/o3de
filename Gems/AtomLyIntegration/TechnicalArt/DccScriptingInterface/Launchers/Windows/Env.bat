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

:: Sets up environment for Lumberyard DCC tools and code access

:: Skip initialization if already completed
IF "%DCCSI_ENV_INIT%"=="1" GOTO :END_OF_FILE

echo.
echo _____________________________________________________________________
echo.
echo ~    Setting up Default LY DCC Scripting Interface Environment ...
echo _____________________________________________________________________
echo.

:: Store current dir
%~d0
cd %~dp0
PUSHD %~dp0

IF "%DCCSI_LAUNCHERS_PATH%"=="" (set DCCSI_LAUNCHERS_PATH=%~dp0)
echo     DCCSI_LAUNCHERS_PATH = %DCCSI_LAUNCHERS_PATH%

:: add to the PATH
SET PATH=%DCCSI_LAUNCHERS_PATH%;%PATH%

:: This maps up to the \Dev folder
IF "%DEV_REL_PATH%"=="" (set DEV_REL_PATH=..\..\..\..)
echo     DEV_REL_PATH = %DEV_REL_PATH%

IF "%LY_PROJECT%"=="" (
    ECHO     !!LY_PROJECT NOT defined!!
    for %%a in (%CD%..\..\..) do set LY_PROJECT=%%~na
    )
echo     LY_PROJECT = %LY_PROJECT%

:: set up the default project path (dccsi)
CD /D ..\..\
IF "%LY_PROJECT_PATH%"=="" (set LY_PROJECT_PATH=%CD%)
echo     LY_PROJECT_PATH = %LY_PROJECT_PATH%

IF "%ABS_PATH%"=="" (set ABS_PATH=%CD%)
echo     ABS_PATH = %ABS_PATH%

:: Save current directory and change to target directory
pushd %ABS_PATH%

:: Change to root Lumberyard dev dir
CD /d %LY_PROJECT_PATH%\%DEV_REL_PATH%
set LY_DEV=%CD%
echo     LY_DEV = %LY_DEV%
:: Restore original directory
popd

:: dcc scripting interface gem path
set DCCSIG_PATH=%LY_DEV%\Gems\AtomLyIntegration\TechnicalArt\DccScriptingInterface
echo     DCCSIG_PATH = %DCCSIG_PATH%

:: Change to root dir
CD /D %DCCSIG_PATH%

:: Constant Vars (Global)
:: global debug (propogates)
IF "%DCCSI_GDEBUG%"=="" (set DCCSI_GDEBUG=false)
echo     DCCSI_GDEBUG = %DCCSI_GDEBUG%
:: initiates debugger connection
IF "%DCCSI_DEV_MODE%"=="" (set DCCSI_DEV_MODE=false)
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

set MAYA_PROJECT=%LY_PROJECT_PATH%
echo     MAYA_PROJECT = %MAYA_PROJECT%

:: add to the PATH
SET PATH=%DCCSIG_PATH%;%PATH%

:: dcc python api path
set DCCSI_AZPY_PATH=%DCCSIG_PATH%\azpy
echo     DCCSI_AZPY_PATH = %DCCSI_AZPY_PATH%

:: per-dcc sdk patj
set DCCSI_SDK_PATH=%DCCSIG_PATH%\SDK
echo     DCCSI_SDK_PATH = %DCCSI_SDK_PATH%

set DCCSI_LOG_PATH=%DCCSIG_PATH%\.temp\logs
echo     DCCSI_LOG_PATH = %DCCSI_LOG_PATH%
echo.

:: PY version Major
IF "%DCCSI_PY_VERSION_MAJOR%"=="" (set DCCSI_PY_VERSION_MAJOR=3)
echo     DCCSI_PY_VERSION_MAJOR = %DCCSI_PY_VERSION_MAJOR%

:: PY version Major
IF "%DCCSI_PY_VERSION_MINOR%"=="" (set DCCSI_PY_VERSION_MINOR=7)
echo     DCCSI_PY_VERSION_MINOR = %DCCSI_PY_VERSION_MINOR%

IF "%DCCSI_PY_VERSION_RELEASE%"=="" (set DCCSI_PY_VERSION_RELEASE=5)
echo     DCCSI_PY_VERSION_RELEASE = %DCCSI_PY_VERSION_RELEASE%

:: shared location for 64bit python 3.7 DEV location
set DCCSI_PYTHON_PATH=%DCCSIG_PATH%\3rdParty\Python
echo     DCCSI_PYTHON_PATH = %DCCSI_PYTHON_PATH%

:: add access to a Lib location that matches the py version (3.7.x)
:: switch this for other python version like maya (2.7.x)
IF "%DCCSI_PYTHON_LIB_PATH%"=="" (set DCCSI_PYTHON_LIB_PATH=%DCCSI_PYTHON_PATH%\Lib\%DCCSI_PY_VERSION_MAJOR%.x\%DCCSI_PY_VERSION_MAJOR%.%DCCSI_PY_VERSION_MINOR%.x\site-packages)
echo     DCCSI_PYTHON_LIB_PATH = %DCCSI_PYTHON_LIB_PATH%

:: add to the PATH
SET PATH=%DCCSI_PYTHON_LIB_PATH%;%PATH%

set DCCSI_PYTHON_ROOT=%LY_DEV%\Tools\Python\%DCCSI_PY_VERSION_MAJOR%.%DCCSI_PY_VERSION_MINOR%.%DCCSI_PY_VERSION_RELEASE%
echo     DCCSI_PYTHON_ROOT = %DCCSI_PYTHON_ROOT%

:: shared location for Lumberyard 64bit python 3.x location
set DCCSI_PYTHON_INSTALL=%DCCSI_PYTHON_ROOT%\windows
echo     DCCSI_PYTHON_INSTALL = %DCCSI_PYTHON_INSTALL%

:: add to the PATH
SET PATH=%DCCSI_PYTHON_INSTALL%;%PATH%

set LY_PY_INTERNAL=%DCCSI_PYTHON_ROOT%\internal\site-packages\windows
echo     LY_PY_INTERNAL = %LY_PY_INTERNAL% 

:: add to the PATH
SET PATH=%LY_PY_INTERNAL%;%PATH%

:: shared location for 64bit python 3.7 BASE location 
set DCCSI_PY_BASE=%DCCSI_PYTHON_INSTALL%\python.exe
echo     DCCSI_PY_BASE = %DCCSI_PY_BASE%
 
:: shared location for 64bit python 3.7 BASE location 
set DCCSI_PY_DCCSI=%DCCSI_LAUNCHERS_PATH%\Launch_pyBASE.bat
echo     DCCSI_PY_DCCSI = %DCCSI_PY_DCCSI%

:: maya sdk path
set DCCSI_SDK_MAYA_PATH=%DCCSI_SDK_PATH%\Maya
echo     DCCSI_SDK_MAYA_PATH = %DCCSI_SDK_MAYA_PATH%

set MAYA_MODULE_PATH=%DCCSI_SDK_MAYA_PATH%;%MAYA_MODULE_PATH%
echo     MAYA_MODULE_PATH = %MAYA_MODULE_PATH%

:: Default Maya Version
IF "%DCCSI_MAYA_VERSION%"=="" (set DCCSI_MAYA_VERSION=2020)
echo     DCCSI_MAYA_VERSION = %DCCSI_MAYA_VERSION%

:: Maya File Paths, etc
:: https://knowledge.autodesk.com/support/maya/learn-explore/caas/CloudHelp/cloudhelp/2015/ENU/Maya/files/Environment-Variables-File-path-variables-htm.html
set MAYA_LOCATION=%ProgramFiles%\Autodesk\Maya%DCCSI_MAYA_VERSION%
echo     MAYA_LOCATION = %MAYA_LOCATION%

set MAYA_BIN_PATH=%MAYA_LOCATION%\bin\
echo     MAYA_BIN_PATH = %MAYA_BIN_PATH%

:: these improve the boot up time
IF "%MAYA_DISABLE_CIP%"=="" (set MAYA_DISABLE_CIP=1)
echo     MAYA_DISABLE_CIP = %MAYA_DISABLE_CIP%

IF "%MAYA_DISABLE_CER%"=="" (set MAYA_DISABLE_CER=1)
echo     MAYA_DISABLE_CER = %MAYA_DISABLE_CER%

IF "%MAYA_DISABLE_CLIC_IPM%"=="" (set MAYA_DISABLE_CLIC_IPM=1)
echo     MAYA_DISABLE_CLIC_IPM = %MAYA_DISABLE_CLIC_IPM%

IF "%DCCSI_MAYA_SET_CALLBACKS%"=="" (set DCCSI_MAYA_SET_CALLBACKS=false)
echo     DCCSI_MAYA_SET_CALLBACKS = %DCCSI_MAYA_SET_CALLBACKS%

:: setting this to 1 should further improve boot time (I think)
::IF "%MAYA_NO_CONSOLE_WINDOW%"=="" (set MAYA_NO_CONSOLE_WINDOW=0)
::echo     MAYA_NO_CONSOLE_WINDOW = %MAYA_NO_CONSOLE_WINDOW%

:: shared location for 64bit DCCSI_PY_MAYA python 2.7 DEV location
set DCCSI_PY_MAYA=%MAYA_LOCATION%\bin\mayapy.exe
echo     DCCSI_PY_MAYA = %DCCSI_PY_MAYA%

:: azpy Maya plugins access
:: our path
set DCCSI_MAYA_PLUG_IN_PATH=%DCCSI_SDK_MAYA_PATH%\plugins
:: also attached to maya's built-it env var
set MAYA_PLUG_IN_PATH=%DCCSI_MAYA_PLUG_IN_PATH%;MAYA_PLUG_IN_PATH
echo     DCCSI_MAYA_PLUG_IN_PATH = %DCCSI_MAYA_PLUG_IN_PATH%

:: azpy Maya shelves
:: our path
set DCCSI_MAYA_SHELF_PATH=%DCCSI_SDK_MAYA_PATH%\Prefs\Shelves
set MAYA_SHELF_PATH=%DCCSI_MAYA_SHELF_PATH%
echo     DCCSI_MAYA_SHELF_PATH = %DCCSI_MAYA_SHELF_PATH%

:: azpy Maya icons
:: our path
set DCCSI_MAYA_XBMLANGPATH=%DCCSI_SDK_MAYA_PATH%\Prefs\icons
:: also attached to maya's built-it env var
set XBMLANGPATH=%DCCSI_MAYA_XBMLANGPATH%;%XBMLANGPATH%
echo     DCCSI_MAYA_XBMLANGPATH = %DCCSI_MAYA_XBMLANGPATH%

:: azpy root Maya boostrap, userSetup.py access
:: our path
set DCCSI_MAYA_SCRIPT_PATH=%DCCSI_SDK_MAYA_PATH%\Scripts
:: also attached to maya's built-it env var
set MAYA_SCRIPT_PATH=%DCCSI_MAYA_SCRIPT_PATH%;%MAYA_SCRIPT_PATH%
echo     DCCSI_MAYA_SCRIPT_PATH = %DCCSI_MAYA_SCRIPT_PATH%

:: azpy Maya Mel scripts
:: our path
set DCCSI_MAYA_SCRIPT_MEL_PATH=%DCCSI_SDK_MAYA_PATH%\Scripts\Mel
:: also attached to maya's built-it env var
set MAYA_SCRIPT_PATH=%DCCSI_MAYA_SCRIPT_MEL_PATH%;%MAYA_SCRIPT_PATH%
echo     DCCSI_MAYA_SCRIPT_MEL_PATH = %DCCSI_MAYA_SCRIPT_MEL_PATH%

:: azpy Maya Py scripts
:: our path
set DCCSI_MAYA_SCRIPT_PY_PATH=%DCCSI_SDK_MAYA_PATH%\Scripts\Python
:: also attached to maya's built-it env var
set MAYA_SCRIPT_PATH=%DCCSI_MAYA_SCRIPT_PY_PATH%;%MAYA_SCRIPT_PATH%
echo     DCCSI_MAYA_SCRIPT_PY_PATH = %DCCSI_MAYA_SCRIPT_PY_PATH%

:: azpy add all python related paths to PYTHONPATH
::set PYTHONPATH=%DCCSIG_PATH%;%DCCSI_AZPY_PATH%;%DCCSI_SDK_MAYA_PATH%;%DCCSI_MAYA_SCRIPT_PATH%;%DCCSI_MAYA_SCRIPT_PY_PATH%;%PYTHONPATH%
set PYTHONPATH=%LY_PY_INTERNAL%;%DCCSIG_PATH%;%DCCSI_PYTHON_LIB_PATH%;%DCCSI_MAYA_SCRIPT_PATH%;%DCCSI_MAYA_SCRIPT_PY_PATH%;%PYTHONPATH%
echo     PYTHONPATH = %PYTHONPATH%

:: build path
IF "%TAG_LY_BUILD_PATH%"=="" (set TAG_LY_BUILD_PATH=windows_vs2019)
echo     TAG_LY_BUILD_PATH = %TAG_LY_BUILD_PATH%

IF "%LY_BUILD_PATH%"=="" (set LY_BUILD_PATH=%LY_DEV%\%TAG_LY_BUILD_PATH%)
echo     LY_BUILD_PATH = %LY_BUILD_PATH%

:: Substance Designer
:: maya sdk path
set DCCSI_SUBSTANCE_PATH=%DCCSI_SDK_PATH%\Substance
echo     DCCSI_SUBSTANCE_PATH = %DCCSI_SUBSTANCE_PATH%
:: https://docs.substance3d.com/sddoc/project-preferences-107118596.html#ProjectPreferences-ConfigurationFile
:: Path to .exe, "C:\Program Files\Allegorithmic\Substance Designer\Substance Designer.exe"
set SUBSTANCE_PATH="%ProgramFiles%\Allegorithmic\Substance Designer"
echo     SUBSTANCE_PATH = %SUBSTANCE_PATH%

:: default config
set SUBSTANCE_CFG_PATH=%LY_PROJECT_PATH%\DCCsi_default.sbscfg
echo     SUBSTANCE_CFG_PATH = %SUBSTANCE_CFG_PATH%

:: WingIDE version Major
IF "%DCCSI_WING_VERSION_MAJOR%"=="" (set DCCSI_WING_VERSION_MAJOR=7)
echo     DCCSI_WING_VERSION_MAJOR = %DCCSI_WING_VERSION_MAJOR%

:: WingIDE version Major
IF "%DCCSI_WING_VERSION_MINOR%"=="" (set DCCSI_WING_VERSION_MINOR=1)
echo     DCCSI_WING_VERSION_MINOR = %DCCSI_WING_VERSION_MINOR%

:: put project env variables/paths here
set WINGHOME=%PROGRAMFILES(X86)%\Wing Pro %DCCSI_WING_VERSION_MAJOR%.%DCCSI_WING_VERSION_MINOR%
echo     WINGHOME = %WINGHOME%

:: add to the PATH
SET PATH=%WINGHOME%;%PATH%

:: ide and debugger plugs
set DCCSI_PY_DEFAULT=%DCCSI_PY_BASE%

:: add prefered python to the PATH
SET PATH=%DCCSIG_PATH%;%DCCSI_AZPY_PATH%;%DCCSI_PY_DEFAULT%;%PATH%
echo     PATH = %PATH%

:: Change to root dir
CD /D %ABS_PATH%

:: if the user has set up a custom env call it
IF EXIST "%~dp0Env_Dev.bat" CALL %~dp0Env_Dev.bat

echo.
echo _____________________________________________________________________
echo.
echo ~    Default DCCsi Environment ...
echo _____________________________________________________________________
echo.

:: Set flag so we don't initialize dccsi environment twice
SET DCCSI_ENV_INIT=1
GOTO END_OF_FILE

:: Return to starting directory
POPD

:END_OF_FILE