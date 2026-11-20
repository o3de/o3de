#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

<#
.SYNOPSIS
    Adds (or removes) Start menu and desktop shortcuts for a portable O3DE install.

.DESCRIPTION
    The portable equivalent of the shortcuts the MSI creates, for the current user only.
    No administrator rights required. Run it again after moving the folder.

.PARAMETER Desktop
    Also add a desktop shortcut.

.PARAMETER Uninstall
    Remove the shortcuts created for this version.
#>

[CmdletBinding()]
param(
    [switch]$Desktop,
    [switch]$Uninstall
)

$ErrorActionPreference = 'Stop'

$EngineRoot = $PSScriptRoot
$BinDir = Join-Path $EngineRoot 'bin\Windows\profile\Default'

$EngineInfo = Get-Content -Raw (Join-Path $EngineRoot 'engine.json') | ConvertFrom-Json
$Version = $EngineInfo.display_version
if ([string]::IsNullOrEmpty($Version) -or $Version -eq '00.00') {
    $Version = $EngineInfo.version
}

$StartMenuDir = Join-Path ([Environment]::GetFolderPath('Programs')) "O3DE $Version"
$DesktopShortcut = Join-Path ([Environment]::GetFolderPath('Desktop')) "O3DE $Version.lnk"

if ($Uninstall) {
    if (Test-Path $StartMenuDir) {
        Remove-Item -Recurse -Force $StartMenuDir
    }
    if (Test-Path $DesktopShortcut) {
        Remove-Item -Force $DesktopShortcut
    }
    Write-Host "Removed O3DE $Version shortcuts"
    exit 0
}

$Shell = New-Object -ComObject WScript.Shell

function New-Shortcut([string]$Path, [string]$Target, [string]$Description) {
    $Shortcut = $Shell.CreateShortcut($Path)
    $Shortcut.TargetPath = $Target
    $Shortcut.WorkingDirectory = $BinDir
    $Shortcut.IconLocation = "$Target,0"
    $Shortcut.Description = $Description
    $Shortcut.Save()
}

New-Item -ItemType Directory -Force -Path $StartMenuDir | Out-Null
New-Shortcut (Join-Path $StartMenuDir "O3DE $Version.lnk") (Join-Path $BinDir 'o3de.exe') 'O3DE Project Manager'
New-Shortcut (Join-Path $StartMenuDir "O3DE Editor $Version.lnk") (Join-Path $BinDir 'Editor.exe') 'O3DE Editor'
Write-Host "Added Start menu shortcuts in $StartMenuDir"

if ($Desktop) {
    New-Shortcut $DesktopShortcut (Join-Path $BinDir 'o3de.exe') 'O3DE Project Manager'
    Write-Host "Added desktop shortcut $DesktopShortcut"
}
