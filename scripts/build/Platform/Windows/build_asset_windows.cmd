@ECHO OFF
REM
REM Copyright (c) Contributors to the Open 3D Engine Project.
REM For complete copyright and license terms please see the LICENSE at the root of this distribution.
REM
REM SPDX-License-Identifier: Apache-2.0 OR MIT
REM
REM

CALL "%~dp0build_windows.cmd"
IF NOT %ERRORLEVEL%==0 GOTO :error

CALL "%~dp0asset_windows.cmd"
IF NOT %ERRORLEVEL%==0 GOTO :error

EXIT /b 0

:error
EXIT /b 1