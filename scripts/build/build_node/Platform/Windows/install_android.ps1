<#
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
#>

choco install -y android-sdk

$Env:ANDROID_HOME = "C:\Android\android-sdk"
setx ANDROID_HOME "C:\Android\android-sdk"

$android_packages = '"platforms;android-28" "platforms;android-29" "platforms;android-30"'
$googleplay_packages = '"extras;google;market_apk_expansion" "extras;google;market_licensing"'
$build_tools = '"build-tools;30.0.2" "tools"'
$ndk = '"ndk;21.4.7075529" "ndk;23.1.7779620" "ndk;25.1.8937393'

$sdkmanager = "C:\Android\android-sdk\tools\bin\sdkmanager.bat"
Start-Process -FilePath $sdkmanager -ArgumentList $android_packages -NoNewWindow -Wait
Start-Process -FilePath $sdkmanager -ArgumentList $googleplay_packages -NoNewWindow -Wait
Start-Process -FilePath $sdkmanager -ArgumentList $build_tools -NoNewWindow -Wait
Start-Process -FilePath $sdkmanager -ArgumentList $ndk -NoNewWindow -Wait

Write-Host "Installing Gradle and Ninja"
Import-Module C:\ProgramData\chocolatey\helpers\chocolateyInstaller.psm1 #Grade needs a custom installer due to being hardcoded to C:\Programdata in Chocolatey
$packageName = 'gradle'
$version = '7.0'
$checksum = '81003F83B0056D20EEDF48CDDD4F52A9813163D4BA185BCF8ABD34B8EEEA4CBD'
$url = "https://services.gradle.org/distributions/gradle-$version-all.zip"
$installDir = "C:\Gradle"

Install-ChocolateyZipPackage $packageName $url $installDir -Checksum $checksum -ChecksumType 'sha256'

$gradle_home = Join-Path $installDir "$packageName-$version"
$gradle_bat = Join-Path $gradle_home 'bin/gradle.bat'

Install-ChocolateyEnvironmentVariable "GRADLE_BUILD_HOME" $gradle_home 'Machine'

choco install -y ninja --version=1.10.0 --package-parameters="/installDir:C:\Ninja"
