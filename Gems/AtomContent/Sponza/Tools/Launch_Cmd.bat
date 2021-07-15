@echo off
REM 
REM Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
REM 
REM SPDX-License-Identifier: Apache-2.0 OR MIT
REM
REM
<<<<<<< HEAD:Gems/AtomTressFX/Tools/Launch_Cmd.bat
:: Set up and run project CMD prompt for TAtomTressFX Gem
=======

@echo off
:: Set up and run LY Python CMD prompt
:: Sets up the DccScriptingInterface_Env,
>>>>>>> development:Gems/AtomContent/Sponza/Tools/Launch_Cmd.bat
:: Puts you in the CMD within the dev environment

:: Set up window
TITLE AtomTressFX (Hair) Interface Cmd
:: Use obvious color to prevent confusion (Grey with Yellow Text)
COLOR 8E

%~d0
cd %~dp0
PUSHD %~dp0

:: Keep changes local
SETLOCAL enableDelayedExpansion

CALL %~dp0\..\Project_Env.bat

echo.
echo _____________________________________________________________________
echo.
echo ~    AtomTressFX (Hair) Interface CMD ...
echo _____________________________________________________________________
echo.

:: Create command prompt with environment
CALL %windir%\system32\cmd.exe

ENDLOCAL

:: Return to starting directory
POPD

:END_OF_FILE