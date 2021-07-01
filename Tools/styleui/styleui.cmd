@ECHO OFF
REM 
REM Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
REM 
REM SPDX-License-Identifier: Apache-2.0 OR MIT
REM
REM

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

