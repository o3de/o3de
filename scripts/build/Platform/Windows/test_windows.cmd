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

REM Find the CTEST_RUN_FLAGS from cmake's cache variables, then replace the $<CONFIG> with the current configuration
FOR /F "tokens=2 delims==" %%V IN ('cmake -N -LA . ^| findstr /I CTEST_RUN_FLAGS:STRING') do (
    SET CTEST_RUN_FLAGS=%%V
)
SET CTEST_RUN_FLAGS=%CTEST_RUN_FLAGS:$<CONFIG>=!CONFIGURATION!%

REM Run ctest
ECHO [ci_build] ctest !CTEST_RUN_FLAGS! !CTEST_OPTIONS!
ctest !CTEST_RUN_FLAGS! !CTEST_OPTIONS!
IF NOT %ERRORLEVEL%==0 GOTO :popd_error

POPD
EXIT /b 0

:popd_error
POPD

:error
EXIT /b 1