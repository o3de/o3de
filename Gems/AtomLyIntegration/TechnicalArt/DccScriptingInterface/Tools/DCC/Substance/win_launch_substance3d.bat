@echo off
REM
REM Copyright (c) Contributors to the Open 3D Engine Project.
REM For complete copyright and license terms please see the LICENSE at the root of this distribution.
REM
REM SPDX-License-Identifier: Apache-2.0 OR MIT
REM
REM

:: Store current dir
%~d0
cd %~dp0
PUSHD %~dp0

SETLOCAL ENABLEDELAYEDEXPANSION

:: if the user has set up a custom env call it
IF EXIST "%~dp0Env_Dev.bat" CALL %~dp0Env_Dev.bat

CALL %~dp0..\..\Dev\Windows\Env_O3DE_Core.bat

:: add to the PATH here (this is global)
SET PATH=%PATH_O3DE_BIN%;%PATH_DCCSIG%;%PATH%

CALL %~dp0..\..\Dev\Windows\Env_DCC_Substance.bat

:: add to the PATH here (this is global)
SET PATH=%PATH_O3DE_BIN%;%PATH_DCCSIG%;%PATH%

:: ide and debugger plug
set "DCCSI_PY_DEFAULT=%DCCSI_SUBSTANCE_PYTHON%\python.exe"
echo     DCCSI_PY_DEFAULT = %DCCSI_PY_DEFAULT%

:: Some IDEs like Wing, may in some cases need acess directly to the exe to operate correctly
set "DCCSI_PY_IDE=%DCCSI_SUBSTANCE_PYTHON%\python.exe"
echo     DCCSI_PY_IDE = %DCCSI_PY_IDE%

SET PATH=%DCCSI_SUBSTANCE_PYTHON%;%DCCSI_PY_IDE%;%DCCSI_PY_DEFAULT%;%PATH%

:: the next line sets up too much, I believe causes boot failure with DCC apps with python
::CALL %~dp0..\Env_O3DE_Python.bat

:: shared location for 64bit python 3.7 DEV location
:: this defines a DCCsi sandbox for lib site-packages by version
:: <O3DE>\Gems\AtomLyIntegration\TechnicalArt\DccScriptingInterface\3rdParty\Python\Lib
set "PATH_DCCSI_PYTHON=%PATH_DCCSIG%\3rdParty\Python"
echo     PATH_DCCSI_PYTHON = %PATH_DCCSI_PYTHON%

:: add access to a Lib location that matches the py version (example: 3.7.x)
:: switch this for other python versions like maya (2.7.x)
IF "%PATH_DCCSI_PYTHON_LIB%"=="" (set "PATH_DCCSI_PYTHON_LIB=%PATH_DCCSI_PYTHON%\Lib\%DCCSI_PY_VERSION_MAJOR%.x\%DCCSI_PY_VERSION_MAJOR%.%DCCSI_PY_VERSION_MINOR%.x\site-packages")
echo     PATH_DCCSI_PYTHON_LIB = %PATH_DCCSI_PYTHON_LIB%

:: add to the PATH
SET PATH=%PATH_DCCSI_PYTHON_LIB%;%PATH%

:: add all python related paths to PYTHONPATH for package imports
set PYTHONPATH=%DCCSI_SUBSTANCE_SCRIPTS%;%PATH_O3DE_TECHART_GEMS%;%PATH_DCCSIG%;%PATH_DCCSI_PYTHON_LIB%;%PYTHONPATH%

:: if the user has set up a custom env call it
IF EXIST "%~dp0Env_Dev.bat" CALL %~dp0Env_Dev.bat

echo.
echo _____________________________________________________________________
echo.
echo Launching Substance Designer for O3DE DCCsi...
echo _____________________________________________________________________
echo.

echo     DCCSI_PY_VERSION_MAJOR = %DCCSI_PY_VERSION_MAJOR%
echo     DCCSI_PY_VERSION_MINOR = %DCCSI_PY_VERSION_MINOR%
echo     DCCSI_PY_VERSION_RELEASE = %DCCSI_PY_VERSION_RELEASE%
echo     DCCSI_SUBSTANCE_LOCATION = %DCCSI_SUBSTANCE_LOCATION%

echo.
echo     PATH = %PATH%
echo.
echo     PYTHONPATH = %PYTHONPATH%
echo.

:: Default to the right version of Maya if we can detect it... and launch
set command=""%DCCSI_SUBSTANCE_EXE%" --config-file %DCCSI_SUBSTANCE_CFG%"

IF EXIST "%DCCSI_SUBSTANCE_LOCATION%\Adobe Substance 3D Designer.exe" (
    start "" "%DCCSI_SUBSTANCE_LOCATION%\Adobe Substance 3D Designer.exe" --config-file %~dp0o3de_dccsi.sbscfg %*
)
::ENDLOCAL

:: Restore previous directory
POPD

:END_OF_FILE
