@ECHO OFF
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

SETLOCAL EnableDelayedExpansion

CALL %~dp0env_windows.cmd

IF NOT EXIST %OUTPUT_DIRECTORY% (
    ECHO [ci_build] Error: $OUTPUT_DIRECTORY was not found
    GOTO :error
)
PUSHD %OUTPUT_DIRECTORY%

REM Override the temporary directory used by wix to the EBS volume
SET "WIX_TEMP=!WORKSPACE!/temp/wix"
IF NOT EXIST "%WIX_TEMP%" (
    MKDIR %WIX_TEMP%
)

REM Make sure we are using the CMake version of CPack and not the one that comes with chocolaty
IF "%LY_CMAKE_PATH%"=="" (
    for /f %%i in ('where cmake') do SET "CMAKE_EXE_PATH=%%i"
    for %%F in ("%CMAKE_EXE_PATH%") do SET "CMAKE_INSTALL_PATH=%%~dpF"
) ELSE (
    SET "CMAKE_INSTALL_PATH=%LY_CMAKE_PATH%\"
)

REM Run cpack
ECHO [ci_build] "%CMAKE_INSTALL_PATH%cpack" -C %CONFIGURATION%
"%CMAKE_INSTALL_PATH%cpack"  -C %CONFIGURATION%
IF NOT %ERRORLEVEL%==0 GOTO :popd_error

POPD
EXIT /b 0

:popd_error
POPD

:error
EXIT /b 1
