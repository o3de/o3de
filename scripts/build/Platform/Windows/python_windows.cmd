@ECHO OFF
REM
REM Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
REM 
REM SPDX-License-Identifier: Apache-2.0 OR MIT
REM
REM

SETLOCAL EnableDelayedExpansion

ECHO [ci_build] python/python.cmd -u %SCRIPT_PATH% %SCRIPT_PARAMETERS%
CALL python/python.cmd -u %SCRIPT_PATH% %SCRIPT_PARAMETERS%

EXIT /b %ERRORLEVEL%