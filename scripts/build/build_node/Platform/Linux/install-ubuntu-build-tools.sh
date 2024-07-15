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

#
# Get Ubuntu version data
#
UBUNTU_DISTRO="$(lsb_release -sc)"
UBUNTU_VER="$(lsb_release -sr)"
if [ "$UBUNTU_DISTRO" == "bionic" ] || [ "$UBUNTU_DISTRO" == "focal" ] || [ "$UBUNTU_DISTRO" == "jammy" ] || [ "$UBUNTU_DISTRO" == "noble" ]
then
    echo "Setup for Ubuntu $UBUNTU_VER LTS ($UBUNTU_DISTRO)"
else
    echo "Unsupported OS version - $UBUNTU_DISTRO"
    exit 1
fi

#
# Add Ubuntu universe repo
#
apt install software-properties-common
add-apt-repository universe

#
# Install curl if its not installed
#
curl --version >/dev/null 2>&1
if [ $? -ne 0 ]
then
    echo "Installing curl"
    apt-get install curl -y
fi

#
# Add the kitware repository for cmake if necessary
#
KITWARE_REPO_COUNT=$(cat /etc/apt/sources.list | grep ^deb | grep https://apt.kitware.com/ubuntu/ | wc -l)
MIN_CMAKE_VER=3.22

if [ $KITWARE_REPO_COUNT -eq 0 ]
then
    echo Adding Kitware Repository for CMake (Min version $MIN_CMAKE_VER)

    wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | sudo tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null
    CMAKE_DEB_REPO="'deb https://apt.kitware.com/ubuntu/ $UBUNTU_DISTRO main'"

    # Add the appropriate kitware repository to apt
    if [ "$UBUNTU_DISTRO" == "bionic" ] || [ "$UBUNTU_DISTRO" == "focal" ]
    then
        apt-add-repository "deb https://apt.kitware.com/ubuntu/ $UBUNTU_DISTRO main"
    elif [ "$UBUNTU_DISTRO" == "jammy" ]
    then
        # Ubuntu 22.04 already has an acceptable version of cmake 3.22.1 (https://packages.ubuntu.com/jammy/cmake)
        echo "Ubuntu 22.04's cmake package already at version 3.22.1 (Which is greater than $MIN_CMAKE_VER)"
    elif [ "$UBUNTU_DISTRO" == "noble" ]
    then
        # Ubuntu 24.04 already has an acceptable version of cmake 3.28.3 (https://packages.ubuntu.com/noble/cmake)
        echo "Ubuntu 24.04's cmake package already at version 3.28.3 (Which is greater than $MIN_CMAKE_VER)"
    fi
else
    echo  Kitware Repository repo already set
fi

echo "Installing cmake"
apt-get install cmake  -y


#
# Add Amazon Corretto repository to install the necessary JDK for Jenkins and Android
#

CORRETTO_REPO_COUNT=$(cat /etc/apt/sources.list | grep ^dev | grep https://apt.corretto.aws | wc -l)

if [ $CORRETTO_REPO_COUNT -eq 0 ]
then
    echo Adding Corretto Repository for JDK
    
    wget -O- https://apt.corretto.aws/corretto.key | apt-key add - 
    add-apt-repository 'deb https://apt.corretto.aws stable main'
else
    echo Corretto repo already set
fi

#
# Add the repository for ROS2
#
ROS2_REPO_COUNT=$(cat /etc/apt/sources.list | grep ^deb | grep http://packages.ros.org/ros2/ubuntu | wc -l)

if [ $ROS2_REPO_COUNT -eq 0 ]
then
    echo Adding ROS2 Repository
    curl -sSL https://raw.githubusercontent.com/ros/rosdistro/master/ros.key -o /usr/share/keyrings/ros-archive-keyring.gpg
    echo "deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/ros-archive-keyring.gpg] http://packages.ros.org/ros2/ubuntu $UBUNTU_DISTRO main" | sudo tee /etc/apt/sources.list.d/ros2.list > /dev/null
else
    echo ROS2 repo already set
fi

#
# Finally, update the sources repos
#
apt-get update
