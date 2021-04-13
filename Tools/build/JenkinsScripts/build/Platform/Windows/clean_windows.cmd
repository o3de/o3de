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

IF DEFINED CLEAN_ASSETS (
    ECHO [ci_build] CLEAN_ASSETS option set
    FOR %%P in (%CMAKE_LY_PROJECTS%) do (
        IF EXIST %%P\Cache (
            ECHO [ci_build] Deleting "%%P\Cache"
            DEL /s /q /f %%P\Cache 1>nul
        )
    )    
)

IF DEFINED CLEAN_OUTPUT_DIRECTORY (
    ECHO [ci_build] CLEAN_OUTPUT_DIRECTORY option set
    IF EXIST %OUTPUT_DIRECTORY% (
        ECHO [ci_build] Deleting "%OUTPUT_DIRECTORY%"
        DEL /s /q /f %OUTPUT_DIRECTORY% 1>nul
    )
)