@echo off

REM 
REM  All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
REM  its licensors.
REM 
REM  For complete copyright and license terms please see the LICENSE at the root of this
REM  distribution (the "License"). All use of this software is governed by the License,
REM  or, if provided, by the license below or the license accompanying this file. Do not
REM  remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
REM  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
REM 

PUSHD "%~dp0"

SET CWD="%~dp0"
SET EXEPATH141="../../../../Bin64vc141/PythonBindingsExample.exe"
SET EXEPATH142="../../../../Bin64vc142/PythonBindingsExample.exe"
SET EXEPATH=""

IF EXIST %EXEPATH141% (
    SET EXEPATH=%EXEPATH141% 
) ELSE (
    IF EXIST %EXEPATH142% (
        SET EXEPATH=%EXEPATH142%
    ) ELSE (
        ECHO PythonBindingsExample.exe not found.
    )
)
IF /I %EXEPATH% EQU "" (
    ECHO [FAILED] Could not run tests since a build of PythonBindingsExample.exe is missing
    GOTO exit_app
)

ECHO Testing basics of tool Python bindings in %CWD%

%EXEPATH% --file test_hello_tool.py
IF %ERRORLEVEL% EQU 0 (
    ECHO [WORKED] test_hello_tool.py
) ELSE ( 
    ECHO [FAILED] test_hello_tool.py with %ERRORLEVEL%
    GOTO exit_app
)

%EXEPATH% --file test_framework.py --arg entity
IF %ERRORLEVEL% EQU 0 (
    ECHO [WORKED] test_framework.py --arg entity
) ELSE ( 
    ECHO [FAILED] test_framework.py --arg entity with %ERRORLEVEL%
    GOTO exit_app
)

%EXEPATH% --file test_framework.py --arg math
IF %ERRORLEVEL% EQU 0 (
    ECHO [WORKED] test_framework.py --arg math
) ELSE ( 
    ECHO [FAILED] test_framework.py --arg math with %ERRORLEVEL%
    GOTO exit_app
)

:exit_app
POPD
