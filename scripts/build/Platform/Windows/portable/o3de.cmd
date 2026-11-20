@ECHO OFF
REM
REM Copyright (c) Contributors to the Open 3D Engine Project.
REM For complete copyright and license terms please see the LICENSE at the root of this distribution.
REM
REM SPDX-License-Identifier: Apache-2.0 OR MIT
REM
REM

REM Launches the O3DE Project Manager from a portable install. The Project Manager takes a few
REM seconds to show its window, so this console stays open with a message until it does, and
REM a second launch while it is starting or running brings the existing window forward instead
REM of starting another copy.

SETLOCAL

SET "PROJECT_MANAGER=%~dp0bin\Windows\profile\Default\o3de.exe"
SET "O3DE_ARGS=%*"

IF NOT EXIST "%PROJECT_MANAGER%" (
    ECHO O3DE Project Manager not found at %PROJECT_MANAGER%
    PAUSE
    EXIT /b 1
)

TITLE O3DE
ECHO Starting the O3DE Project Manager, this can take a few seconds...

SET "LAUNCH=$pm = $env:PROJECT_MANAGER;"
SET "LAUNCH=%LAUNCH% $running = @(Get-Process -Name o3de -ErrorAction SilentlyContinue | Where-Object { $_.Path -eq $pm });"
SET "LAUNCH=%LAUNCH% if ($running.Count -gt 0) {"
SET "LAUNCH=%LAUNCH%     Write-Host 'It is already running, switching to it.';"
SET "LAUNCH=%LAUNCH%     (New-Object -ComObject WScript.Shell).AppActivate($running[0].Id) | Out-Null;"
SET "LAUNCH=%LAUNCH%     Start-Sleep -Seconds 2; exit 0"
SET "LAUNCH=%LAUNCH% };"
SET "LAUNCH=%LAUNCH% if ($env:O3DE_ARGS) { $p = Start-Process -FilePath $pm -ArgumentList $env:O3DE_ARGS -PassThru }"
SET "LAUNCH=%LAUNCH% else { $p = Start-Process -FilePath $pm -PassThru };"
SET "LAUNCH=%LAUNCH% $deadline = (Get-Date).AddSeconds(60);"
SET "LAUNCH=%LAUNCH% while (-not $p.HasExited -and $p.MainWindowHandle -eq 0 -and (Get-Date) -lt $deadline) {"
SET "LAUNCH=%LAUNCH%     Start-Sleep -Milliseconds 250; $p.Refresh()"
SET "LAUNCH=%LAUNCH% }"

powershell.exe -NoProfile -ExecutionPolicy Bypass -Command "%LAUNCH%"
