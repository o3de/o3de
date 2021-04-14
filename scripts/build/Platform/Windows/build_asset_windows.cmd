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

CALL "%~dp0build_windows.cmd"
IF NOT %ERRORLEVEL%==0 GOTO :error

CALL "%~dp0asset_windows.cmd"
IF NOT %ERRORLEVEL%==0 GOTO :error

EXIT /b 0

:error
EXIT /b 1