<#
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
choco install corretto8jdk -y --ia INSTALLDIR="c:\jdk8" # Custom directory to handle cases where whitespace in the path is not quote wrapped

# Install CMake
choco install cmake -y --installargs 'ADD_CMAKE_TO_PATH=System'

# Install Windows Installer XML toolkit (WiX)
choco install wixtoolset -y
