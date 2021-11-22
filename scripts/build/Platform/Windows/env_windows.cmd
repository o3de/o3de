@ECHO OFF
REM
REM Copyright (c) Contributors to the Open 3D Engine Project.
REM For complete copyright and license terms please see the LICENSE at the root of this distribution.
REM
REM SPDX-License-Identifier: Apache-2.0 OR MIT
REM
REM

REM To get recursive folder creation
SETLOCAL EnableExtensions EnableDelayedExpansion

where /Q cmake
IF NOT %ERRORLEVEL%==0 (
    ECHO [ci_build] CMake not found
    GOTO :error
)

IF NOT "%COMMAND_CWD%"=="" (
    ECHO [ci_build] Changing CWD to %COMMAND_CWD%
    CD %COMMAND_CWD%
)

ENDLOCAL

REM Jenkins does not defined TMP
IF "%TMP%"=="" (
    IF "%WORKSPACE%"=="" (
        SET TMP=%APPDATA%\Local\Temp
        SET TEMP=%APPDATA%\Local\Temp
    ) ELSE (
        SET TMP=%WORKSPACE%\Temp
        SET TEMP=%WORKSPACE%\Temp
    )
)

EXIT /b 0

:error
ENDLOCAL
EXIT /b 1