<#
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
#>
Import-Module C:\ProgramData\chocolatey\helpers\chocolateyInstaller.psm1 

#Android SDK Command line tools needs a custom installer due the latest command like version not being available in the Chocolatey repository
$androidSdkPackageName = 'androidsdk'
$androidSdkVersion = '11076708_latest'
$androidSdkUrl = "https://dl.google.com/android/repository/commandlinetools-win-$androidSdkVersion.zip"
$androidSdkChecksum = '4d6931209eebb1bfb7c7e8b240a6a3cb3ab24479ea294f3539429574b1eec862'
$androidSdkInstallDir = "C:\AndroidSdk\cmdline-tools"
$androidSdkLatestDir = "$androidSdkInstallDir\latest"
Install-ChocolateyZipPackage $androidSdkPackageName $androidSdkUrl $androidSdkInstallDir -Checksum $androidSdkChecksum -ChecksumType 'sha256'
Install-ChocolateyEnvironmentVariable "ANDROID_SDK_ROOT" "C:\AndroidSdk" -VariableType 'Machine'
# Rename the Android SDK folder 'cmdline-tools' to match what the command line tools looks like normally in a full AndroidSDK install
Rename-Item -Path "$androidSdkInstallDir\cmdline-tools" "$androidSdkLatestDir"

# Set package versions
$android_packages = '"platforms;android-28" "platforms;android-29" "platforms;android-30"'
$googleplay_packages = '"extras;google;market_apk_expansion" "extras;google;market_licensing"'
$build_tools = '"build-tools;30.0.2" "build-tools;34.0.0" "tools"'
$ndk = '"ndk;21.4.7075529" "ndk;23.1.7779620" "ndk;25.1.8937393" "ndk;25.2.9519653"'

Write-Host "Installing Android SDK packages..."
$sdkmanager = "$androidSdkLatestDir\bin\sdkmanager.bat"
Write-Host "Installing Android platform packages: $android_packages"
Start-Process -FilePath $sdkmanager -ArgumentList $android_packages -RedirectStandardOutput "NUL" -RedirectStandardError "$host.Streams.Error" -NoNewWindow -Wait
Write-Host "Installing Google Play packages: $googleplay_packages"
Start-Process -FilePath $sdkmanager -ArgumentList $googleplay_packages -RedirectStandardOutput "NUL" -RedirectStandardError "$host.Streams.Error" -NoNewWindow -Wait
Write-Host "Installing Build Tools: $build_tools"
Start-Process -FilePath $sdkmanager -ArgumentList $build_tools -RedirectStandardOutput "NUL" -RedirectStandardError "$host.Streams.Error" -NoNewWindow -Wait
Write-Host "Installing Android NDK packages: $ndk"
Start-Process -FilePath $sdkmanager -ArgumentList $ndk -NoNewWindow -Wait
# Set the NDK environment
Install-ChocolateyEnvironmentVariable "LY_NDK_DIR" "C:\AndroidSdk\ndk\25.1.8937393" -VariableType 'Machine'

$gradle_version = '8.7'
$gradle_checksum = '194717442575a6f96e1c1befa2c30e9a4fc90f701d7aee33eb879b79e7ff05c0'
Write-Host "Installing Gradle $gradle_version"
#Gradle needs a custom installer due to being hardcoded to C:\Programdata in Chocolatey
Import-Module C:\ProgramData\chocolatey\helpers\chocolateyInstaller.psm1 
$packageName = 'gradle'
$url = "https://services.gradle.org/distributions/gradle-$gradle_version-all.zip"
$installDir = "C:\Gradle"

Install-ChocolateyZipPackage $packageName $url $installDir -Checksum $gradle_checksum -ChecksumType 'sha256'

$gradle_home = Join-Path $installDir "$packageName-$gradle_version"
$gradle_bat = Join-Path $gradle_home 'bin\gradle.bat'

Install-ChocolateyEnvironmentVariable "GRADLE_HOME" $gradle_home -VariableType 'Machine'
Install-ChocolateyEnvironmentVariable "GRADLE_BUILD_HOME" $gradle_home -VariableType 'Machine'
Install-ChocolateyPath "$gradle_home\bin" -PathType 'Machine'
Install-BinFile -Name 'gradle' -Path $gradle_bat

$ninja_version = 1.10.0
Write-Host "Installing Ninja $ninja_version"
choco install -y ninja --version=$ninja_version --package-parameters="/installDir:C:\Ninja"
Install-ChocolateyPath "C:\Ninja" -PathType 'Machine'

refreshenv
