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

where /Q cmake
IF NOT %ERRORLEVEL%==0 (
    IF "%LY_CMAKE_PATH%"=="" (SET LY_CMAKE_PATH=%LY_3RDPARTY_PATH%/CMake/3.19.1/Windows/bin)
    IF NOT EXIST !LY_CMAKE_PATH! (
        ECHO [ci_build] CMake path not found
        GOTO :error
    )
    PATH !LY_CMAKE_PATH!;!PATH!
    where /Q cmake
    IF NOT !ERRORLEVEL!==0 (
        ECHO [ci_build] CMake not found
        GOTO :error
    )
)

EXIT /b 0

:error
EXIT /b 1