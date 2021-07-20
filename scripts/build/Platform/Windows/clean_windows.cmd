@ECHO OFF
REM
REM Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
REM 
REM SPDX-License-Identifier: Apache-2.0 OR MIT
REM
REM

SETLOCAL EnableDelayedExpansion

REM Jenkins defines environment variables for parameters and passes "false" to variables 
REM that are not set. Here we clear them if they are false so we can also just define them
REM from command line
IF "%CLEAN_ASSETS%"=="false" (
    set CLEAN_ASSETS=
)
IF "%CLEAN_OUTPUT_DIRECTORY%"=="false" (
    set CLEAN_OUTPUT_DIRECTORY=
)

IF DEFINED CLEAN_ASSETS (
    ECHO [ci_build] CLEAN_ASSETS option set
    FOR %%P in (%CMAKE_LY_PROJECTS%) do (
        IF EXIST %%P\Cache (
            ECHO [ci_build] Deleting "%%P\Cache"
            DEL /s /q /f %%P\Cache 1>nul
        )
    )
)

REM If the node label changes, we issue a clean output since node changes can change SDK/CMake/toolchains/etc
SET LAST_CONFIGURE_NODE_LABEL_FILE=ci_last_node_label.txt
IF DEFINED NODE_LABEL (
    IF EXIST %OUTPUT_DIRECTORY% (
        PUSHD %OUTPUT_DIRECTORY%
        IF EXIST !LAST_CONFIGURE_NODE_LABEL_FILE! (
            FOR /F "delims=" %%x in (%LAST_CONFIGURE_NODE_LABEL_FILE%) DO SET LAST_NODE_LABEL=%%x
        ) ELSE (
            SET LAST_NODE_LABEL=
        )
        REM Detect if the node label has changed
        IF !LAST_NODE_LABEL! NEQ !NODE_LABEL! (
            ECHO [ci_build] Last run was done with node label "!LAST_NODE_LABEL!", new node label is "!NODE_LABEL!", forcing CLEAN_OUTPUT_DIRECTORY
            SET CLEAN_OUTPUT_DIRECTORY=1
        )
        POPD
    )
)

IF DEFINED CLEAN_OUTPUT_DIRECTORY (
    ECHO [ci_build] CLEAN_OUTPUT_DIRECTORY option set
    IF EXIST %OUTPUT_DIRECTORY% (
        ECHO [ci_build] Deleting "%OUTPUT_DIRECTORY%"
        DEL /s /q /f %OUTPUT_DIRECTORY% 1>nul
    )
)

IF NOT EXIST "%OUTPUT_DIRECTORY%" (
    MKDIR %OUTPUT_DIRECTORY%.
)
REM Save the node label
PUSHD %OUTPUT_DIRECTORY%
ECHO !NODE_LABEL!> !LAST_CONFIGURE_NODE_LABEL_FILE!
POPD
