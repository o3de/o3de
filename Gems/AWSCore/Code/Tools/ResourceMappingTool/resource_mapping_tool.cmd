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

SET VIRTUALENV_PATH=
SET RESOURCE_MAPPING_DIR=%CMD_DIR%
SET LOCAL_PYTHONPATH=%VIRTUALENV_PATH%\Scripts\python.exe

%LOCAL_PYTHONPATH% %RESOURCE_MAPPING_DIR%\resource_mapping_tool.py %* && exit 0
exit 1
