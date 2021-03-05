<#
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#>

choco install -y android-sdk

$Env:ANDROID_HOME = "C:\Android\android-sdk"
setx ANDROID_HOME "C:\Android\android-sdk"

$apis_packages = '"add-ons;addon-google_apis-google-19" "add-ons;addon-google_apis-google-21" "add-ons;addon-google_apis-google-22" "add-ons;addon-google_apis-google-23" "add-ons;addon-google_apis-google-24"'
$android_packages = '"platforms;android-19" "platforms;android-21" "platforms;android-22" "platforms;android-23" "platforms;android-24" "platforms;android-25" "platforms;android-26" "platforms;android-27" "platforms;android-28" "platforms;android-29" "platforms;android-30"'
$googleplay_packages = '"extras;google;market_apk_expansion" "extras;google;market_licensing"'
$build_tools = '"build-tools;30.0.2" "tools"'

$sdkmanager = "C:\Android\android-sdk\tools\bin\sdkmanager.bat"
Start-Process -FilePath $sdkmanager -ArgumentList $apis_packages -NoNewWindow -Wait
Start-Process -FilePath $sdkmanager -ArgumentList $android_packages -NoNewWindow -Wait
Start-Process -FilePath $sdkmanager -ArgumentList $googleplay_packages -NoNewWindow -Wait
Start-Process -FilePath $sdkmanager -ArgumentList $build_tools -NoNewWindow -Wait

Write-Host "Installing Gradle and Ninja"
Import-Module C:\ProgramData\chocolatey\helpers\chocolateyInstaller.psm1 #Grade needs a custom installer due to being hardcoded to C:\Programdata in Chocolatey
$packageName = 'gradle'
$version = '5.6.4'
$checksum = 'ABC10BCEDB58806E8654210F96031DB541BCD2D6FC3161E81CB0572D6A15E821'
$url = "https://services.gradle.org/distributions/gradle-$version-all.zip"
$installDir = "C:\Gradle"

Install-ChocolateyZipPackage $packageName $url $installDir -Checksum $checksum -ChecksumType 'sha256'

$gradle_home = Join-Path $installDir "$packageName-$version"
$gradle_bat = Join-Path $gradle_home 'bin/gradle.bat'

Install-ChocolateyEnvironmentVariable "GRADLE_HOME" $gradle_home 'Machine'

choco install -y ninja --version=1.10.0 --package-parameters="/installDir:C:\Ninja"