<#
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
#>

choco install visualstudio2022buildtools --version=117.4.1.0 --package-parameters "--config $pwd\vs2022bt.vsconfig" -y
choco install visualstudio2019buildtools --version=16.11.21 --package-parameters "--config $pwd\vs2019bt.vsconfig" -y

Write-Host "Installing downstream dependancies"
choco install -y dotnet3.5 

# This is a custom install of the Win10 SDK debugger which was not included in VS2019/2017. This resolves an issue where the older DbgHelp library was being referenced from C:\Windows\System32\
# More details here: https://docs.microsoft.com/en-us/windows/win32/debug/calling-the-dbghelp-library

Import-Module C:\ProgramData\chocolatey\helpers\chocolateyInstaller.psm1 
$packageName = 'windows-sdk-10.1'
$featureName = 'OptionId.WindowsDesktopDebuggers'
$installerType = 'EXE'
$url = 'https://download.microsoft.com/download/4/2/2/42245968-6A79-4DA7-A5FB-08C0AD0AE661/windowssdk/winsdksetup.exe'
$checksum = '2E28117E82B4D02FE30D564B835ACE9976612609271265872F20F2256A9C506B'
$checksumType = 'sha256'
$silentArgs = "/Quiet /NoRestart /features $featureName /Log ""$env:temp\${packageName}_$([Guid]::NewGuid().ToString('D')).log"""
$validExitCodes = @(0,3010)
Install-ChocolateyPackage $packageName $installerType $silentArgs $url -checksum $checksum -checksumType $checksumType -validExitCodes $validExitCodes
