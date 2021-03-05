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

REM Delete output directory if CLEAN_OUTPUT_DIRECTORY env variable is set
IF "%CLEAN_OUTPUT_DIRECTORY%"=="true" (
    IF EXIST Cache (
        ECHO [ci_build] CLEAN_OUTPUT_DIRECTORY option set with value "%CLEAN_OUTPUT_DIRECTORY%"
        ECHO [ci_build] Deleting "Cache"
        DEL /s /q /f Cache
    )
)

IF NOT EXIST %OUTPUT_DIRECTORY% (
    ECHO [ci_build] Error: %OUTPUT_DIRECTORY% was not found
    GOTO :error
)
PUSHD %OUTPUT_DIRECTORY%

IF NOT EXIST %ASSET_PROCESSOR_BINARY% (
    ECHO [ci_build] Error: %ASSET_PROCESSOR_BINARY% was not found
    GOTO :error
)

FOR %%P in (%CMAKE_LY_PROJECTS%) do (
    ECHO [ci_build] %ASSET_PROCESSOR_BINARY% %ASSET_PROCESSOR_OPTIONS% --gamefolder=%%P --platforms=%ASSET_PROCESSOR_PLATFORMS%
    %ASSET_PROCESSOR_BINARY% %ASSET_PROCESSOR_OPTIONS% --gamefolder=%%P --platforms=%ASSET_PROCESSOR_PLATFORMS%
    IF NOT !ERRORLEVEL!==0 GOTO :popd_error
)

POPD
EXIT /b 0

:popd_error
POPD

:error
EXIT /b 1