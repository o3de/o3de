@echo off
REM 
REM Copyright (c) Contributors to the Open 3D Engine Project.
REM For complete copyright and license terms please see the LICENSE at the root of this distribution.
REM
REM SPDX-License-Identifier: Apache-2.0 OR MIT
REM
REM

:: Sets up extended environment for O3DE and DCCsi python

:: Skip initialization if already completed
IF "%DCCSI_ENV_PY_INIT%"=="1" GOTO :END_OF_FILE

:: Store current dir
%~d0
cd %~dp0
PUSHD %~dp0

CALL %~dp0\Env_Core.bat

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
set O3DE_PYTHON_INSTALL=%O3DE_DEV%\python
echo     O3DE_PYTHON_INSTALL = %O3DE_PYTHON_INSTALL%

:: location for O3DE python 3.7 location 
:: Note, many DCC tools (like Maya) include thier own python interpretter
:: Some DCC apps may not operate correctly if PYTHONHOME is set (this is definitely the case with Maya)
:: Be aware the python.cmd below does set PYTHONHOME
set DCCSI_PY_BASE=%O3DE_PYTHON_INSTALL%\python.cmd
echo     DCCSI_PY_BASE = %DCCSI_PY_BASE%

CALL %O3DE_PYTHON_INSTALL%\get_python_path.bat

:: Some IDEs like Wing, may in some cases need acess directly to the exe to operate correctly
IF "%DCCSI_PY_IDE%"=="" (set DCCSI_PY_IDE=%O3DE_PYTHONHOME%\python.exe)
echo     DCCSI_PY_IDE = %DCCSI_PY_IDE%

:: add to the PATH
SET PATH=%O3DE_PYTHON_INSTALL%;%O3DE_PYTHONHOME%;%DCCSI_PY_IDE%;%PATH%

:: add all python related paths to PYTHONPATH for package imports
set PYTHONPATH=%DCCSIG_PATH%;%DCCSI_PYTHON_LIB_PATH%;%O3DE_BUILD_PATH%;%PYTHONPATH%
echo     PYTHONPATH = %PYTHONPATH%

:: Set flag so we don't initialize dccsi environment twice
SET DCCSI_ENV_PY_INIT=1
GOTO END_OF_FILE

:: Return to starting directory
POPD

:END_OF_FILE
