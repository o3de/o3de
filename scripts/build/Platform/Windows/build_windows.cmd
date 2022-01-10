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

IF NOT EXIST "%OUTPUT_DIRECTORY%" (
    MKDIR %OUTPUT_DIRECTORY%.
)

SET SOURCE_DIRECTORY=%CD%
PUSHD %OUTPUT_DIRECTORY%

ECHO [ci_build] cmake --version
cmake --version
IF ERRORLEVEL 1 (
    ECHO [ci_build] CMAKE not found!
    exit /b 1
)

REM Compute half the amount of processors so some jobs can run
SET /a HALF_PROCESSORS = NUMBER_OF_PROCESSORS / 2

SET LAST_CONFIGURE_CMD_FILE=ci_last_configure_cmd.txt
SET CONFIGURE_CMD=cmake %SOURCE_DIRECTORY% %CMAKE_OPTIONS% %EXTRA_CMAKE_OPTIONS%
IF NOT "%CMAKE_LY_PROJECTS%"=="" (
    SET CONFIGURE_CMD=!CONFIGURE_CMD! -DLY_PROJECTS="%CMAKE_LY_PROJECTS%"
)
IF NOT EXIST CMakeCache.txt (
    ECHO [ci_build] First run, generating
    SET RUN_CONFIGURE=1
) ELSE IF NOT EXIST %LAST_CONFIGURE_CMD_FILE% (
    ECHO [ci_build] Last run command not found, generating
    SET RUN_CONFIGURE=1
) ELSE (
    REM Detect if the input has changed
    FOR /F "delims=" %%x in (%LAST_CONFIGURE_CMD_FILE%) DO SET LAST_CMD=%%x
    IF !LAST_CMD! NEQ !CONFIGURE_CMD! (
        ECHO [ci_build] Last run command different, generating
        SET RUN_CONFIGURE=1
    )
)
IF DEFINED RUN_CONFIGURE (
    ECHO [ci_build] %CONFIGURE_CMD%
    %CONFIGURE_CMD%
    IF NOT !ERRORLEVEL!==0 GOTO :error
    ECHO !CONFIGURE_CMD!> %LAST_CONFIGURE_CMD_FILE%
)

ECHO [ci_build] cmake --build . --target %CMAKE_TARGET% --config %CONFIGURATION% %CMAKE_BUILD_ARGS% -- %CMAKE_NATIVE_BUILD_ARGS%
cmake --build . --target %CMAKE_TARGET% --config %CONFIGURATION% %CMAKE_BUILD_ARGS% -- %CMAKE_NATIVE_BUILD_ARGS%
IF NOT %ERRORLEVEL%==0 GOTO :error

POPD
EXIT /b 0

:error
POPD
EXIT /b 1
