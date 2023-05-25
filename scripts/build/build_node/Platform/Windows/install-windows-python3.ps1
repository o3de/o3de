<#
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
#>

Write-Host "Install Python"
choco install -y python3 --version=3.10.5

Write-Host "Ensure Python paths are set"
[Environment]::SetEnvironmentVariable(
    "PATH",
    [Environment]::GetEnvironmentVariable("PATH", [EnvironmentVariableTarget]::Machine) + ";C:\Python310;C:\Python310\Scripts",
    [EnvironmentVariableTarget]::Machine)

Write-Host "Installing packages" # requirements.txt hould be in the "Platforms\Common" folder
pip3 install -r "$pwd\requirements.txt"
