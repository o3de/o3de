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

:: Sets up extended environment for O3DE and DCCsi python

:: Skip initialization if already completed
IF "%DCCSI_ENV_PY_INIT%"=="1" GOTO :END_OF_FILE

:: Store current dir
%~d0
cd %~dp0
PUSHD %~dp0

CALL %~dp0\Env_Core.bat

::SETLOCAL ENABLEDELAYEDEXPANSION

echo.
echo _____________________________________________________________________
echo.
echo ~    O3DE DCCsi Python Environment ...
echo _____________________________________________________________________
echo.

:: Python Version
:: Ideally these are set to match the O3DE python distribution
:: <O3DE>\python\runtime
IF "%DCCSI_PY_VERSION_MAJOR%"=="" (set DCCSI_PY_VERSION_MAJOR=3)
echo     DCCSI_PY_VERSION_MAJOR = %DCCSI_PY_VERSION_MAJOR%

:: PY version Major
IF "%DCCSI_PY_VERSION_MINOR%"=="" (set DCCSI_PY_VERSION_MINOR=7)
echo     DCCSI_PY_VERSION_MINOR = %DCCSI_PY_VERSION_MINOR%

IF "%DCCSI_PY_VERSION_RELEASE%"=="" (set DCCSI_PY_VERSION_RELEASE=10)
echo     DCCSI_PY_VERSION_RELEASE = %DCCSI_PY_VERSION_RELEASE%

:: shared location for 64bit python 3.7 DEV location
:: this defines a DCCsi sandbox for lib site-packages by version
:: <O3DE>\Gems\AtomLyIntegration\TechnicalArt\DccScriptingInterface\3rdParty\Python\Lib
set DCCSI_PYTHON_PATH=%DCCSIG_PATH%\3rdParty\Python
echo     DCCSI_PYTHON_PATH = %DCCSI_PYTHON_PATH%

:: add access to a Lib location that matches the py version (example: 3.7.x)
:: switch this for other python versions like maya (2.7.x)
IF "%DCCSI_PYTHON_LIB_PATH%"=="" (set DCCSI_PYTHON_LIB_PATH=%DCCSI_PYTHON_PATH%\Lib\%DCCSI_PY_VERSION_MAJOR%.x\%DCCSI_PY_VERSION_MAJOR%.%DCCSI_PY_VERSION_MINOR%.x\site-packages)
echo     DCCSI_PYTHON_LIB_PATH = %DCCSI_PYTHON_LIB_PATH%

:: add to the PATH
SET PATH=%DCCSI_PYTHON_LIB_PATH%;%PATH%

:: shared location for default O3DE python location
set DCCSI_PYTHON_INSTALL=%LY_DEV%\Python
echo     DCCSI_PYTHON_INSTALL = %DCCSI_PYTHON_INSTALL%

:: location for O3DE python 3.7 location 
set DCCSI_PY_BASE=%DCCSI_PYTHON_INSTALL%\python.cmd
echo     DCCSI_PY_BASE = %DCCSI_PY_BASE%

:: ide and debugger plug
set DCCSI_PY_DEFAULT=%DCCSI_PY_BASE%

:: Wing and other IDEs probably prefer access directly to the python.exe
set DCCSI_PY_IDE=%DCCSI_PYTHON_INSTALL%\runtime\python-3.7.10-rev1-windows\python
echo     DCCSI_PY_IDE = %DCCSI_PY_IDE%

set DCCSI_PY_IDE_PACKAGES=%DCCSI_PY_IDE%\Lib\site-packages
echo     DCCSI_PY_IDE_PACKAGES = %DCCSI_PY_IDE_PACKAGES% 

:: add to the PATH
SET PATH=%DCCSI_PYTHON_INSTALL%;%DCCSI_PY_IDE%;%DCCSI_PY_IDE_PACKAGES%;%PATH%

:: add all python related paths to PYTHONPATH for package imports
set PYTHONPATH=%DCCSIG_PATH%;%DCCSI_PYTHON_LIB_PATH%;%LY_BUILD_PATH%;%PYTHONPATH%
echo     PYTHONPATH = %PYTHONPATH%

::ENDLOCAL

:: Set flag so we don't initialize dccsi environment twice
SET DCCSI_ENV_PY_INIT=1
GOTO END_OF_FILE

:: Return to starting directory
POPD

:END_OF_FILE