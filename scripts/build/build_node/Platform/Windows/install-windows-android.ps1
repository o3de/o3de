<#
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
#>

Write-Host "Installing Android SDK"
choco install -y android-sdk
[Environment]::SetEnvironmentVariable("ANDROID_HOME", "C:\Android\android-sdk", [EnvironmentVariableTarget]::Machine)

# Set package versions
$android_packages = '"platforms;android-28" "platforms;android-29" "platforms;android-30"'
$googleplay_packages = '"extras;google;market_apk_expansion" "extras;google;market_licensing"'
$build_tools = '"build-tools;30.0.2" "build-tools;34.0.0" "tools"'
$ndk = '"ndk;21.4.7075529" "ndk;23.1.7779620" "ndk;25.1.8937393" "ndk;25.2.9519653"'

Write-Host "Installing Android SDK packages"
$sdkmanager = "C:\Android\android-sdk\tools\bin\sdkmanager.bat"
Start-Process -FilePath $sdkmanager -ArgumentList $android_packages -NoNewWindow -Wait
Start-Process -FilePath $sdkmanager -ArgumentList $googleplay_packages -NoNewWindow -Wait
Start-Process -FilePath $sdkmanager -ArgumentList $build_tools -NoNewWindow -Wait
Start-Process -FilePath $sdkmanager -ArgumentList $ndk -NoNewWindow -Wait
# Set the NDK environment
[Environment]::SetEnvironmentVariable("LY_NDK_DIR", "C:\AndroidSdk\ndk\25.1.8937393", [EnvironmentVariableTarget]::Machine)

Write-Host "Installing Gradle"
$gradle_version = '7.0'
$gradle_checksum = '81003F83B0056D20EEDF48CDDD4F52A9813163D4BA185BCF8ABD34B8EEEA4CBD'

#Gradle needs a custom installer due to being hardcoded to C:\Programdata in Chocolatey
Import-Module C:\ProgramData\chocolatey\helpers\chocolateyInstaller.psm1 
$packageName = 'gradle'
$url = "https://services.gradle.org/distributions/gradle-$gradle_version-all.zip"
$installDir = "C:\Gradle"

Install-ChocolateyZipPackage $packageName $url $installDir -Checksum $gradle_checksum -ChecksumType 'sha256'

$gradle_home = Join-Path $installDir "$packageName-$gradle_version"
[Environment]::SetEnvironmentVariable("GRADLE_BUILD_HOME", $gradle_home, [EnvironmentVariableTarget]::Machine)
[Environment]::SetEnvironmentVariable(
    "PATH",
    [Environment]::GetEnvironmentVariable("PATH", [EnvironmentVariableTarget]::Machine) + ";$gradle_home\bin",
    [EnvironmentVariableTarget]::Machine)

Write-Host "Installing Ninja"
$ninja_version = 1.10.0
choco install -y ninja --version=$ninja_version --package-parameters="/installDir:C:\Ninja"
[Environment]::SetEnvironmentVariable(
    "PATH",
    [Environment]::GetEnvironmentVariable("PATH", [EnvironmentVariableTarget]::Machine) + ";C:\Ninja",
    [EnvironmentVariableTarget]::Machine)
