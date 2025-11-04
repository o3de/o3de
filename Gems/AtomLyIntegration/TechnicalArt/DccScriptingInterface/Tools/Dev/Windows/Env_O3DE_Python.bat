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

CALL %~dp0\Env_O3DE_Core.bat

echo.
echo _____________________________________________________________________
echo.
echo ~    O3DE DCCsi Python Environment ...
echo _____________________________________________________________________
echo.

:: this is the default env setup for O3DE python
:: we will attempt to avoid causing conflicts in DCC tools with their
:: own python (which is most of them)

:: Python Version
:: Ideally these are set to match the O3DE python distribution
:: <O3DE>\python\runtime

:: need a more reliable way to set these, instead of updating them to match every time python runtime updates
IF "%DCCSI_PY_VERSION_MAJOR%"=="" (set DCCSI_PY_VERSION_MAJOR=3)
echo     DCCSI_PY_VERSION_MAJOR = %DCCSI_PY_VERSION_MAJOR%

:: PY version Major
IF "%DCCSI_PY_VERSION_MINOR%"=="" (set DCCSI_PY_VERSION_MINOR=10)
echo     DCCSI_PY_VERSION_MINOR = %DCCSI_PY_VERSION_MINOR%

IF "%DCCSI_PY_VERSION_RELEASE%"=="" (set DCCSI_PY_VERSION_RELEASE=5)
echo     DCCSI_PY_VERSION_RELEASE = %DCCSI_PY_VERSION_RELEASE%

:: shared location for 64bit python 3.10 DEV location
:: this defines a DCCsi sandbox for lib site-packages by version
:: <O3DE>\Gems\AtomLyIntegration\TechnicalArt\DccScriptingInterface\3rdParty\Python\Lib
set "PATH_DCCSI_PYTHON=%PATH_DCCSIG%\3rdParty\Python"
echo     PATH_DCCSI_PYTHON = %PATH_DCCSI_PYTHON%

:: add access to a Lib location that matches the py version (example: 3.10.x)
:: switch this for other python versions like maya (2.7.x)
IF "%PATH_DCCSI_PYTHON_LIB%"=="" (set "PATH_DCCSI_PYTHON_LIB=%PATH_DCCSI_PYTHON%\Lib\%DCCSI_PY_VERSION_MAJOR%.x\%DCCSI_PY_VERSION_MAJOR%.%DCCSI_PY_VERSION_MINOR%.x\site-packages")
echo     PATH_DCCSI_PYTHON_LIB = %PATH_DCCSI_PYTHON_LIB%

:: we should NOT add to the PATH here (this is global)
:: setting PATH should be move to Launch .bat files 
:::SET PATH=%PATH_DCCSI_PYTHON_LIB%;%PATH%

:: shared location for default O3DE python location
IF "%PATH_O3DE_PYTHON_INSTALL%"=="" (set "PATH_O3DE_PYTHON_INSTALL=%O3DE_DEV%\python")
echo     PATH_O3DE_PYTHON_INSTALL = %PATH_O3DE_PYTHON_INSTALL%

:: location for O3DE python 3.10 location 
:: Note, many DCC tools (like Maya) include thier own python interpreter
:: Some DCC apps may not operate correctly if PYTHONHOME is set (this is definitely the case with Maya)
:: Be aware the python.cmd below does set PYTHONHOME
IF "%DCCSI_PY_BASE%"=="" (set "DCCSI_PY_BASE=%PATH_O3DE_PYTHON_INSTALL%\python.cmd")
echo     DCCSI_PY_BASE = %DCCSI_PY_BASE%

:: will set O3DE_PYTHONHOME location
CALL %PATH_O3DE_PYTHON_INSTALL%\get_python_path.bat

:: ide and debugger plug
IF "%DCCSI_PY_DEFAULT%"=="" (set "DCCSI_PY_DEFAULT=%PATH_O3DE_PYTHON_INSTALL%\python.cmd")
echo     DCCSI_PY_DEFAULT = %DCCSI_PY_DEFAULT%

:: Some IDEs like Wing, may in some cases need acess directly to the exe to operate correctly
:: ide and debugger plug
IF "%DCCSI_PY_IDE%"=="" (set "DCCSI_PY_IDE=%O3DE_PYTHONHOME%\python.exe")
echo     DCCSI_PY_IDE = %DCCSI_PY_IDE%

:: O3DE installs additional python pkgs from 3rd party and other locations
:: See: C:\path\to\o3de\python\runtime\python-x.x.x-revX-windows\python\Lib\site-packages\easy-install.pth

::ENDLOCAL

:: Set flag so we don't initialize dccsi environment twice
SET DCCSI_ENV_PY_INIT=1
GOTO END_OF_FILE

:: Return to starting directory
POPD

:END_OF_FILE
