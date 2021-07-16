@echo off

REM 
REM Copyright (c) Contributors to the Open 3D Engine Project.
REM For complete copyright and license terms please see the LICENSE at the root of this distribution.
REM
REM SPDX-License-Identifier: Apache-2.0 OR MIT
REM
REM

:: Set up and run LY Python CMD prompt
:: Sets up the DccScriptingInterface_Env,
:: Puts you in the CMD within the dev environment

:: Set up window
TITLE Lumberyard DCC Scripting Interface Cmd
:: Use obvious color to prevent confusion (Grey with Yellow Text)
COLOR 8E

%~d0
cd %~dp0

:: Keep changes local
SETLOCAL enableDelayedExpansion

:: This maps up to the \Dev folder
IF "%DEV_REL_PATH%"=="" (set DEV_REL_PATH=..\..\..\..\..\..\..\..\..)

:: Change to root Lumberyard dev dir
:: Don't use the LY_DEV so we can test that ENVAR!!!
CD /d %DEV_REL_PATH%
set Rel_Dev=%CD%
echo     Rel_Dev = %Rel_Dev%
:: Restore original directory
popd

set DCCSI_PYTHON_INSTALL=%Rel_Dev%\Tools\Python\3.7.5\windows

:: add to the PATH
SET PATH=%DCCSI_PYTHON_INSTALL%;%PATH%

:: dcc scripting interface gem path
set DCCSIG_PATH=%Rel_Dev%\Gems\AtomLyIntegration\TechnicalArt\DccScriptingInterface
echo     DCCSIG_PATH = %DCCSIG_PATH%

:: add to the PATH
SET PATH=%DCCSIG_PATH%;%PATH%

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

echo.
echo _____________________________________________________________________
echo.
echo ~    LY DCCsi, DCC Material Converter
echo _____________________________________________________________________
echo.

:: Change to root dir
CD /D %DCCSIG_PATH%

:: add to the PATH
SET PATH=%DCCSIG_PATH%;%PATH%

set PYTHONPATH=%DCCSIG_PATH%;%PYTHONPATH%

CALL %DCCSI_PYTHON_INSTALL%\python.exe "%DCCSIG_PATH%\SDK\Maya\Scripts\Python\kitbash_converter\standalone.py"


ENDLOCAL

:: Return to starting directory
POPD

:END_OF_FILE

exit /b 0
