@ECHO OFF
REM
REM Copyright (c) Contributors to the Open 3D Engine Project.
REM For complete copyright and license terms please see the LICENSE at the root of this distribution.
REM
REM SPDX-License-Identifier: Apache-2.0 OR MIT
REM
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