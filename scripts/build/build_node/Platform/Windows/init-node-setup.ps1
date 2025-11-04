<#
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
#>

# Disable Windows Defender
Set-MpPreference -DisableRealtimeMonitoring $true

# Disable Admin prompts
New-ItemProperty -Path HKLM:Software\Microsoft\Windows\CurrentVersion\policies\system -Name EnableLUA -PropertyType DWord -Value 0 -Force
New-ItemProperty -Path HKLM:Software\Microsoft\Windows\CurrentVersion\policies\system -Name ConsentPromptBehaviorAdmin -PropertyType DWord -Value 0 -Force

# Enable crash reporting (defaults to %LOCALAPPDATA%\CrashDumps as minidumps)
New-Item -Path HKLM:Software\Microsoft\Windows\Windows Error Reporting\LocalDumps

# Increase port number availablity (for P4) 
netsh int ipv4 set dynamicport tcp start=1025 num=64511

# Install Chocolatey and Carbon (local user util)
[System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072; iex ((New-Object System.Net.WebClient).DownloadString('https://chocolatey.org/install.ps1'))
$env:ChocolateyInstall = Convert-Path "$((Get-Command choco).Path)\..\.."   
Import-Module "$env:ChocolateyInstall\helpers\chocolateyProfile.psm1"
choco install Carbon -y
choco install awscli -y
refreshenv

