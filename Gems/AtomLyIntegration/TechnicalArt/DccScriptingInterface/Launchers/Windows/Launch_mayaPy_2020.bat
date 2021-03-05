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

:: Store current directory and change to environment directory so script works in any path.
%~d0
cd %~dp0
PUSHD %~dp0

:: Keep changes local
SETLOCAL enableDelayedExpansion

:: PY version Major
set DCCSI_PY_VERSION_MAJOR=2
echo     DCCSI_PY_VERSION_MAJOR = %DCCSI_PY_VERSION_MAJOR%

:: PY version Major
set DCCSI_PY_VERSION_MINOR=7
echo     DCCSI_PY_VERSION_MINOR = %DCCSI_PY_VERSION_MINOR%

:: Default Maya Version
set MAYA_VERSION=2020
echo     MAYA_VERSION = %MAYA_VERSION%

:: Initialize env
CALL %~dp0\Env.bat

echo ~    Launching LY Dc cScripting Interfacw MayaPy (%MAYA_VERSION%) ...
echo ________________________________________________________________
echo.

:: Start Maya
:: Default to the right version of Maya if we can detect it.
Set MAYA_VP2_DEVICE_OVERRIDE = VirtualDeviceDx11

IF EXIST "%DCCSI_PY_MAYA%" (
   CALL "%DCCSI_PY_MAYA%" %*
) ELSE (
   Where mayapy.exe 2> NUL
   IF ERRORLEVEL 1 (
      echo mayapy.exe could not be found
      pause
   ) ELSE (
      start "" mayapy.exe %*
   )
)

ENDLOCAL

:: Restore previous directory
POPD