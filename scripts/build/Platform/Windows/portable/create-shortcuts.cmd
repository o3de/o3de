@ECHO OFF
REM
REM Copyright (c) Contributors to the Open 3D Engine Project.
REM For complete copyright and license terms please see the LICENSE at the root of this distribution.
REM
REM SPDX-License-Identifier: Apache-2.0 OR MIT
REM
REM

REM Runs create-shortcuts.ps1 without changing the PowerShell execution policy.
REM Usage: create-shortcuts.cmd [-Desktop] [-Uninstall]

powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0create-shortcuts.ps1" %*
