<#
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
#>

Write-Host "Install Python"
choco install -y python3 --version=3.10.11
$pyroot = "C:\Python310"

Write-Host "Ensure Python paths are set"
[Environment]::SetEnvironmentVariable(
    "PATH",
    [Environment]::GetEnvironmentVariable("PATH", [EnvironmentVariableTarget]::Machine) + ";$pyroot;$pyroot\Scripts",
    [EnvironmentVariableTarget]::Machine)

Write-Host "Installing packages" # requirements.txt hould be in the "Platforms\Common" folder
pip3 install -r "$pwd\requirements.txt"

New-Item -ItemType SymbolicLink -Path "$pyroot\python3.exe" -Target "$pyroot\python.exe" # The Jenkins scripts use `python3` (as with linux) for python calls
