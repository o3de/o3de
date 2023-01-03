<#
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
#>

# Install dependancies
choco install -y 7zip
choco install -y procexp
choco install -y windirstat
choco install -y sysinternals

# Install source control apps 
choco install -y git
choco install -y git-lfs
choco install -y p4

Write-Host "Configuring Git"
git config --global "credential.helper" "!aws codecommit credential-helper $@"
git config --global "credential.UseHttpPath" "true"

# Install Java (for Jenkins)
choco install corretto11jdk -y --ia INSTALLDIR="c:\jdk11" # Custom directory to handle cases where whitespace in the path is not quote wrapped

# Install Java (for Android)
choco install corretto8jdk -y --ia INSTALLDIR="c:\jdk8" # Custom directory to handle cases where whitespace in the path is not quote wrapped

# Set JDK_PATH for Android
[Environment]::SetEnvironmentVariable("JDK_PATH", "c:\jdk8", [EnvironmentVariableTarget]::Machine)
    
# Set Java path for Jenkins node agent
[Environment]::SetEnvironmentVariable("JENKINS_JAVA_CMD", "c:\jdk11", [EnvironmentVariableTarget]::Machine)

# Install CMake
choco install cmake --version=3.24.0 -y --installargs 'ADD_CMAKE_TO_PATH=System'

# Install Windows Installer XML toolkit (WiX)
choco install wixtoolset -y
