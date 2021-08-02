@ECHO OFF
REM
REM Copyright (c) Contributors to the Open 3D Engine Project.
REM For complete copyright and license terms please see the LICENSE at the root of this distribution.
REM
REM SPDX-License-Identifier: Apache-2.0 OR MIT
REM
REM

SETLOCAL EnableDelayedExpansion

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
    ECHO [ci_build] %ASSET_PROCESSOR_BINARY% %ASSET_PROCESSOR_OPTIONS% --project-path=%%P --platforms=%ASSET_PROCESSOR_PLATFORMS%
    %ASSET_PROCESSOR_BINARY% %ASSET_PROCESSOR_OPTIONS% --project-path=%%P --platforms=%ASSET_PROCESSOR_PLATFORMS%
    IF NOT !ERRORLEVEL!==0 GOTO :popd_error
)

POPD
EXIT /b 0

:popd_error
POPD

:error
EXIT /b 1