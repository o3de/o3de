@echo off

REM 
REM Copyright (c) Contributors to the Open 3D Engine Project
REM 
REM SPDX-License-Identifier: Apache-2.0 OR MIT
REM For complete copyright and license terms please see the LICENSE at the root of this distribution.
REM
REM


:: Sets ocio and oiio tools for O3DE Color Grading

:: Skip initialization if already completed
IF "%O3DE_ENV_TOOLS_INIT%"=="1" GOTO :END_OF_FILE

:: Store current dir
%~d0
cd %~dp0
PUSHD %~dp0

:: Initialize env
CALL %~dp0\Env_Core.bat
CALL %~dp0\Env_Python.bat

::SETLOCAL ENABLEDELAYEDEXPANSION

echo.
echo _____________________________________________________________________
echo.
echo ~    O3DE Color Grading Tools Env ...
echo _____________________________________________________________________
echo.

::SET oiio_bin=%O3DE_PROJECT_PATH%\Tools\oiio\build\bin\Release
::SET oiiotool=%oiio_bin%\oiiotool.exe

:: Libs that ocio uses
SET vcpkg_bin=%O3DE_DEV%\Tools\ColorGrading\vcpkg\installed\x64-windows\bin
:: add to path
SET PATH=%PATH%;%vcpkg_bin%

SET ocio_bin=%O3DE_DEV%\Tools\ColorGrading\ocio\build\src\OpenColorIO\Release
echo     ocio_bin = %ocio_bin%
:: add to the PATH
SET PATH=%PATH%;%ocio_bin%

IF "%OCIO_APPS%"=="" (set OCIO_APPS=%O3DE_DEV%\Tools\ColorGrading\ocio\build\src\apps)
echo     OCIO_APPS = %OCIO_APPS%

SET ociobakelut=%OCIO_APPS%\ociobakelut\Release
echo     ociobakelut = %ociobakelut%
SET PATH=%PATH%;%ociobakelut%

SET ociocheck=%OCIO_APPS%\ociocheck\Release
SET PATH=%PATH%;%ociocheck%

SET ociochecklut=%OCIO_APPS%\ociochecklut\Release
SET PATH=%PATH%;%ociochecklut%

SET ocioconvert=%OCIO_APPS%\ocioconvert\Release
SET PATH=%PATH%;%ocioconvert%

SET ociodisplay=%OCIO_APPS%\ociodisplay\Release
SET PATH=%PATH%;%ociodisplay%

SET ociolutimage=%OCIO_APPS%\ociolutimage\Release
SET PATH=%PATH%;%ociolutimage%

SET ociomakeclf=%OCIO_APPS%\ociomakeclf\Release
SET PATH=%PATH%;%ociomakeclf%

SET ocioperf=%OCIO_APPS%\ocioperf\Release
SET PATH=%PATH%;%ocioperf%

SET ociowrite=%OCIO_APPS%\ociowrite\Release
SET PATH=%PATH%;%ociowrite%

::ENDLOCAL

:: Set flag so we don't initialize this environment twice
SET O3DE_ENV_TOOLS_INIT=1
GOTO END_OF_FILE

:: Return to starting directory
POPD

:END_OF_FILE