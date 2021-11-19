@ECHO OFF
REM
REM Copyright (c) Contributors to the Open 3D Engine Project.
REM For complete copyright and license terms please see the LICENSE at the root of this distribution.
REM
REM SPDX-License-Identifier: Apache-2.0 OR MIT
REM
REM

where /Q cmake
IF NOT %ERRORLEVEL%==0 (
    ECHO [ci_build] CMake not found
    GOTO :error
)

IF NOT "%COMMAND_CWD%"=="" (
    ECHO [ci_build] Changing CWD to %COMMAND_CWD%
    CD %COMMAND_CWD%
)

REM Jenkins reports MSB8029 when TMP/TEMP is not defined, define a dummy folder
IF NOT "%TMP%"=="" (
    IF NOT "%WORKSPACE_TMP%"=="" (
        SET TMP=%WORKSPACE_TMP%
        SET TEMP=%WORKSPACE_TMP%
    ) ELSE (
        SET TMP=%cd%/temp
        SET TEMP=%cd%/temp
    )
)

EXIT /b 0

:error
EXIT /b 1