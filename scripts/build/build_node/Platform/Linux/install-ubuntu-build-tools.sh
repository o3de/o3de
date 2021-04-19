#!/bin/bash

# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

# This script must be run as root
if [ "`whoami`" != "root" ]
then
    echo "This script must be run as root (sudo)"
    exit 1
fi

#
# Make sure we are installing on a supported ubuntu distro
#
lsb_release -c >/dev/null 2>&1
if [ $? -ne 0 ]
then
    echo This script is only supported on Ubuntu Distros
    exit 1
fi

UBUNTU_DISTRO="`lsb_release -c | awk '{print $2}'`"
if [ "$UBUNTU_DISTRO" == "bionic" ]
then
    echo "Setup for Ubuntu 18.04 LTS ($UBUNTU_DISTRO)"
elif [ "$UBUNTU_DISTRO" == "focal" ]
then
    echo "Setup for Ubuntu 20.04 LTS ($UBUNTU_DISTRO)"
else
    echo "Unsupported version of Ubuntu $UBUNTU_DISTRO"
    exit 1
fi

#
# Always install the latest version of cmake (from kitware)
#
echo Installing the latest version of CMake

# Remove any pre-existing version of cmake 
apt purge --auto-remove cmake -y
wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | sudo tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null
CMAKE_DEB_REPO="'deb https://apt.kitware.com/ubuntu/ $UBUNTU_DISTRO main'"

# Add the appropriate kitware repository to apt
if [ "$UBUNTU_DISTRO" == "bionic" ]
then
    apt-add-repository 'deb https://apt.kitware.com/ubuntu/ bionic main'
elif [ "$UBUNTU_DISTRO" == "focal" ]
then
    apt-add-repository 'deb https://apt.kitware.com/ubuntu/ focal main'
fi
apt-get update

# Install cmake
apt-get install cmake -y


#
# Make sure that Ninja is installed
#
echo Installing Ninja
apt-get install ninja-build -y


echo Build Tools Setup Complete
