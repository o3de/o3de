:: coding:utf-8
:: !/usr/bin/python
::
:: All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
:: its licensors.
::
:: For complete copyright and license terms please see the LICENSE at the root of this
:: distribution (the "License"). All use of this software is governed by the License,
:: or, if provided, by the license below or the license accompanying this file. Do not
:: remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
:: WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
::

@echo off
:: Set up and run LY Python CMD prompt
:: Sets up the DccScriptingInterface_Env,
:: Puts you in the CMD within the dev environment

:: Set up window
TITLE Lumberyard DCC Scripting Interface Cmd
:: Use obvious color to prevent confusion (Grey with Yellow Text)
COLOR 8E

%~d0
cd %~dp0
PUSHD %~dp0

:: Keep changes local
SETLOCAL enableDelayedExpansion

CALL %~dp0\Project_Env.bat

echo.
echo _____________________________________________________________________
echo.
echo ~    LY DCC Scripting Interface CMD ...
echo _____________________________________________________________________
echo.

:: Create command prompt with environment
CALL %windir%\system32\cmd.exe

ENDLOCAL

:: Return to starting directory
POPD

:END_OF_FILE