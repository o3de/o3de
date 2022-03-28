@echo off

REM 
REM Copyright (c) Contributors to the Open 3D Engine Project
REM 
REM SPDX-License-Identifier: Apache-2.0 OR MIT
REM For complete copyright and license terms please see the LICENSE at the root of this distribution.
REM
REM

:: Sets up extended environment for O3DE and DCCsi python

:: Skip initialization if already completed
IF "%O3DE_ENV_PY_INIT%"=="1" GOTO :END_OF_FILE

:: Store current dir
%~d0
cd %~dp0
PUSHD %~dp0

CALL %~dp0\Env_Core.bat

echo.
echo _____________________________________________________________________
echo.
echo ~    O3DE Color Grading Python Env ...
echo _____________________________________________________________________
echo.

:: shared location for default O3DE python location
set DCCSI_PYTHON_INSTALL=%O3DE_DEV%\Python
echo     DCCSI_PYTHON_INSTALL = %DCCSI_PYTHON_INSTALL%

:: Warning, many DCC tools (like Maya) include thier own versioned python interpretter.
:: Some apps may not operate correctly if PYTHONHOME is set/propogated.
:: This is definitely the case with Maya, doing so causes Maya to not boot.
FOR /F "tokens=* USEBACKQ" %%F IN (`%DCCSI_PYTHON_INSTALL%\python.cmd %DCCSI_PYTHON_INSTALL%\get_python_path.py`) DO (SET PYTHONHOME=%%F)
echo     PYTHONHOME - is now the folder containing O3DE python executable
echo     PYTHONHOME = %PYTHONHOME% 

SET PYTHON=%PYTHONHOME%\python.exe

:: location for O3DE python 3.7 location 
set DCCSI_PY_BASE=%DCCSI_PYTHON_INSTALL%\python.cmd
echo     DCCSI_PY_BASE = %DCCSI_PY_BASE%

:: ide and debugger plug
set DCCSI_PY_DEFAULT=%DCCSI_PY_BASE%

set DCCSI_PY_IDE=%PYTHONHOME%
echo     DCCSI_PY_IDE = %DCCSI_PY_IDE%

:: Wing and other IDEs probably prefer access directly to the python.exe
set DCCSI_PY_EXE=%DCCSI_PY_IDE%\python.exe
echo     DCCSI_PY_EXE = %DCCSI_PY_EXE%

set DCCSI_PY_IDE_PACKAGES=%DCCSI_PY_IDE%\Lib\site-packages
echo     DCCSI_PY_IDE_PACKAGES = %DCCSI_PY_IDE_PACKAGES% 

set DCCSI_FEATURECOMMON_SCRIPTS=%O3DE_DEV%\Gems\Atom\Feature\Common\Editor\Scripts
echo     DCCSI_FEATURECOMMON_SCRIPTS = %DCCSI_FEATURECOMMON_SCRIPTS% 

set DCCSI_COLORGRADING_SCRIPTS=%DCCSI_FEATURECOMMON_SCRIPTS%\ColorGrading
echo     DCCSI_COLORGRADING_SCRIPTS = %DCCSI_COLORGRADING_SCRIPTS% 

:: add to the PATH
SET PATH=%DCCSI_PYTHON_INSTALL%;%DCCSI_PY_IDE%;%DCCSI_PY_IDE_PACKAGES%;%DCCSI_PY_EXE%;%DCCSI_COLORGRADING_SCRIPTS%;%PATH%

:: add all python related paths to PYTHONPATH for package imports
set PYTHONPATH=%DCCSIG_PATH%;%DCCSI_PYTHON_LIB_PATH%;%O3DE_BIN_PATH%;%DCCSI_COLORGRADING_SCRIPTS%;%DCCSI_FEATURECOMMON_SCRIPTS%;%PYTHONPATH%
echo     PYTHONPATH = %PYTHONPATH%

:: Set flag so we don't initialize dccsi environment twice
SET O3DE_ENV_PY_INIT=1
GOTO END_OF_FILE

:: Return to starting directory
POPD

:END_OF_FILE
