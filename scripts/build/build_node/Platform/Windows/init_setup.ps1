<#
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
#>

# Disable Windows Defender
Set-MpPreference -DisableRealtimeMonitoring $true

# Disable Admin prompts
New-ItemProperty -Path HKLM:Software\Microsoft\Windows\CurrentVersion\policies\system -Name EnableLUA -PropertyType DWord -Value 0 -Force
New-ItemProperty -Path HKLM:Software\Microsoft\Windows\CurrentVersion\policies\system -Name ConsentPromptBehaviorAdmin -PropertyType DWord -Value 0 -Force

# Increase port number availablity (for P4) 
netsh int ipv4 set dynamicport tcp start=1025 num=64511

# Install Chocolatey and Carbon (local user util)
[System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072; iex ((New-Object System.Net.WebClient).DownloadString('https://chocolatey.org/install.ps1'))
$env:ChocolateyInstall = Convert-Path "$((Get-Command choco).Path)\..\.."   
Import-Module "$env:ChocolateyInstall\helpers\chocolateyProfile.psm1"
choco install Carbon -y
choco install awscli -y
refreshenv

# Grab credentials from parameter store (assumes AWS cli is installed and correct IAM policy is setup)
$az = curl http://169.254.169.254/latest/meta-data/placement/availability-zone -UseBasicParsing
$region = $az.Content -replace ".$"
$username = aws ssm get-parameters --names "shared.builderuser" --region $region --with-decryption | ConvertFrom-Json
$username = $username.Parameters.Value.ToString()
$password = aws ssm get-parameters --names "shared.builderpass" --region $region --with-decryption | ConvertFrom-Json
$password = ConvertTo-SecureString $password.Parameters.Value.ToString() -AsPlainText -Force
$credential = New-Object System.Management.Automation.PSCredential -ArgumentList $username, $password

Write-Host "Adding builder user"
Import-Module 'Carbon'
Install-User -Credential $credential -FullName "$($username)" -Description "Builder account for LY"
Add-GroupMember -Name "Administrators" -Member "$($username)"