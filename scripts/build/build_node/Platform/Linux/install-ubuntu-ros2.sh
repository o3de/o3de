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
# Install curl if its not installed
#
curl --version >/dev/null 2>&1
if [ $? -ne 0 ]
then
    echo "Installing curl"
    apt-get install curl -y
fi

#
# Add Ubuntu universe repo
#
apt install software-properties-common
add-apt-repository universe

#
# Add the repository for ROS2 if necessary
#
ROS2_REPO_COUNT=$(cat /etc/apt/sources.list | grep ^deb | grep http://packages.ros.org/ros2/ubuntu | wc -l)

if [ $KITWARE_REPO_COUNT -eq 0 ]
then
    curl -sSL https://raw.githubusercontent.com/ros/rosdistro/master/ros.key -o /usr/share/keyrings/ros-archive-keyring.gpg
    echo "deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/ros-archive-keyring.gpg] http://packages.ros.org/ros2/ubuntu $(. /etc/os-release && echo $UBUNTU_CODENAME) main" | sudo tee /etc/apt/sources.list.d/ros2.list > /dev/null
    apt-get update
else
    echo ROS2 repo already set
fi
