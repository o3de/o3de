#!/bin/bash

# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

# This script must be run as root
if [[ $EUID -ne 0 ]]
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

UBUNTU_DISTRO="$(lsb_release -c | awk '{print $2}')"
if [ "$UBUNTU_DISTRO" == "bionic" ]
then
    echo "Setup for Ubuntu 18.04 LTS ($UBUNTU_DISTRO)"
elif [ "$UBUNTU_DISTRO" == "focal" ]
then
    echo "Setup for Ubuntu 20.04 LTS ($UBUNTU_DISTRO)"
elif [ "$UBUNTU_DISTRO" == "jammy" ]
then
    echo "Setup for Ubuntu 22.04 LTS ($UBUNTU_DISTRO)"
elif [ "$UBUNTU_DISTRO" == "noble" ]
then
    echo "Setup for Ubuntu 24.04 LTS ($UBUNTU_DISTRO)"
else
    echo "Unsupported version of Ubuntu $UBUNTU_DISTRO"
    exit 1
fi

#
# Setup and get the latest from git if necessary
#
git --version > /dev/null 2>&1
if [ $? -ne 0 ]
then
    echo Setting up latest version of GIT
    add-apt-repository ppa:git-core/ppa -y
    apt-get update
    apt-get install git -y
else
    GIT_VERSION=$(git --version | awk '{print $3}')
    echo Git $GIT_VERSION already Installed. Skipping Git installation
fi

#
# Setup Git-LFS if needed
#
GIT_LFS_PACKAGE_COUNT=$(apt list --installed 2>/dev/null | grep git-lfs/ | wc -l)
if [ $GIT_LFS_PACKAGE_COUNT -eq 0 ]
then
    echo Setting up Git-LFS
    pushd /tmp
    wget https://packagecloud.io/install/repositories/github/git-lfs/script.deb.sh -o script.deb.sh
    rm script.deb.sh
    mv script.deb.sh.1 script.deb.sh
    chmod +x script.deb.sh
    ./script.deb.sh
    sudo apt-get install git-lfs -y
    popd
else
    echo Git LFS already installed. Skipping Git-LFS installation
fi

# Setup GCM if needed
git-credential-manager-core --version > /dev/null 2>&1
if [ $? -ne 0 ]
then
    # Download and setup Git Credential Manager
    GCM_PACKAGE_NAME=gcmcore-linux_amd64.2.0.394.50751.deb
    GCM_PACKAGE_URL=https://github.com/microsoft/Git-Credential-Manager-Core/releases/download/v2.0.394-beta

    echo Installing Git Credential Manager \($GCM_PACKAGE_NAME\)

    pushd /tmp > /dev/null 
    curl --location $GCM_PACKAGE_URL/$GCM_PACKAGE_NAME -o $GCM_PACKAGE_NAME
    dpkg -i $GCM_PACKAGE_NAME
    popd
else
    GCM_VERSION=$(git-credential-manager-core --version)
    echo Git Credential Manager \(GCM\) version $GCM_VERSION already installed. Skipping GCM installation
fi

# Setup pass (password manager) for git-credential-manager
apt-get install pass -y

