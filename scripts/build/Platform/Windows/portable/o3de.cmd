@ECHO OFF
REM
REM Copyright (c) Contributors to the Open 3D Engine Project.
REM For complete copyright and license terms please see the LICENSE at the root of this distribution.
REM
REM SPDX-License-Identifier: Apache-2.0 OR MIT
REM
REM

REM Launches the O3DE Project Manager from a portable install

SETLOCAL

SET "PROJECT_MANAGER=%~dp0bin\Windows\profile\Default\o3de.exe"

IF NOT EXIST "%PROJECT_MANAGER%" (
    ECHO O3DE Project Manager not found at %PROJECT_MANAGER%
    PAUSE
    EXIT /b 1
)

START "" "%PROJECT_MANAGER%" %*
