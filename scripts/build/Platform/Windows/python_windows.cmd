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

ECHO [ci_build] python/python.cmd -u %SCRIPT_PATH% %SCRIPT_PARAMETERS%
CALL python/python.cmd -u %SCRIPT_PATH% %SCRIPT_PARAMETERS%

EXIT /b %ERRORLEVEL%