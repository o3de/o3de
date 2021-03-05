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
REM Original file Copyright Crytek GMBH or its affiliates, used under license.
REM

SETLOCAL
SET CMD_DIR=%~dp0
SET CMD_DIR=%CMD_DIR:~0,-1%
SET SDKS_DIR=%CMD_DIR%\..\..\Code\SDKs

SET PYTHON_DIR=%SDKS_DIR%\Python
IF EXIST %PYTHON_DIR% GOTO PYTHON_READY
ECHO Missing: %PYTHON_DIR%
GOTO :EOF
:PYTHON_READY
SET PYTHON=%PYTHON_DIR%\x64\python.exe

SET STYLEUI_DIR=%CMD_DIR%
SET PYTHONPATH=%STYLEUI_DIR%

%PYTHON% %STYLEUI_DIR%\styleui.py %*

