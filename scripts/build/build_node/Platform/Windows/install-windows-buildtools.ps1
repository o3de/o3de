<#
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
#>

Write-Host "Install source control apps"
choco install -y git
choco install -y git-lfs

Write-Host "Install Java (for Jenkins)"
choco install corretto11jdk -y --ia INSTALLDIR="c:\jdk11" # Custom directory to handle cases where whitespace in the path is not quote wrapped

Write-Host "Install Java (for Android)"
choco install corretto17jdk -y --ia INSTALLDIR="c:\jdk17" # Custom directory to handle cases where whitespace in the path is not quote wrapped

Write-Host "Set JDK_PATH for Android"
[Environment]::SetEnvironmentVariable("JDK_PATH", "c:\jdk17\jdk17.0.12_7", [EnvironmentVariableTarget]::Machine)
    
Write-Host "Set Java path for Jenkins node agent"
[Environment]::SetEnvironmentVariable("JENKINS_JAVA_CMD", "c:\jdk11", [EnvironmentVariableTarget]::Machine)

# Install CMake
choco install cmake --version=3.22.1 -y --installargs 'ADD_CMAKE_TO_PATH=System'

Write-Host "Install Windows Installer XML toolkit (WiX)"
choco install wixtoolset -y
