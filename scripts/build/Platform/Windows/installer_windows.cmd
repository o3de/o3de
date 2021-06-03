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

REM Override the temporary directory used by wix to the workspace
SET "WIX_TEMP=!WORKSPACE!/temp/wix"
IF NOT EXIST "%WIX_TEMP%" (
    MKDIR "%WIX_TEMP%"
)

REM Make sure we are using the CMake version of CPack and not the one that comes with chocolatey
SET CPACK_PATH=
IF "%LY_CMAKE_PATH%"=="" (
    REM quote the paths from 'where' so we can properly tokenize ones in the list with spaces
    FOR /F delims^=^"^ tokens^=1 %%i in ('where /F cpack') DO (
        REM The cpack in chocolatey expects a number supplied with --version so it will error
        "%%i" --version > NUL
        IF !ERRORLEVEL!==0 (
            SET "CPACK_PATH=%%i"
            GOTO :run_cpack
        )
    )
) ELSE (
    SET "CPACK_PATH=%LY_CMAKE_PATH%\cpack.exe"
)

:run_cpack
ECHO [ci_build] "!CPACK_PATH!" --version
"!CPACK_PATH!" --version
IF ERRORLEVEL 1 (
    ECHO [ci_build] CPack not found!
    GOTO :popd_error
)

ECHO [ci_build] "!CPACK_PATH!" -C %CONFIGURATION%
"!CPACK_PATH!"  -C %CONFIGURATION%
IF NOT %ERRORLEVEL%==0 GOTO :popd_error

POPD
EXIT /b 0

:popd_error
POPD

:error
EXIT /b 1
