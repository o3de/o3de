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

echo Configuring environment settings

# Enable coredumps - Will be written to /var/lib/apport/coredump
ulimit -c unlimited

# Configure NTP sync - This is necessary to prevent mtime drift between input and output files 
systemctl stop systemd-timesyncd
systemctl disable systemd-timesyncd
apt install -y chrony

if [ $(curl -LI http://169.254.169.254/latest/meta-data/ -o /dev/null -w '%{http_code}\n' -s) == "200" ]; then
    echo EC2 instance detected. Setting NTP address to AWS NTP
    sed -i '1s/^/server 169.254.169.123 prefer iburst minpoll 4 maxpoll 4\n/' /etc/chrony/chrony.conf
fi
/etc/init.d/chrony restart

echo Installing packages and tools for O3DE development

# Install awscli
./install-ubuntu-awscli.sh
if [ $? -ne 0 ]
then
    echo Error installing AWSCLI
    exit 1
fi

# Install git
./install-ubuntu-git.sh
if [ $? -ne 0 ]
then
    echo Error installing Git
    exit 1
fi

# Install the necessary build tools repos
./install-ubuntu-build-tools.sh
if [ $? -ne 0 ]
then
    echo Error installing ubuntu tool repos
    exit 1
fi

# Install the necessary build tools for android on linux
./install-ubuntu-build-android-tools.sh
if [ $? -ne 0 ]
then
    echo Error installing ubuntu android tools
    exit 1
fi

# Install the packages in the platform list
./install-ubuntu-packages.sh
if [ $? -ne 0 ]
then
    echo Error installing ubuntu packages
    exit 1
fi

# Add mountpoint for Jenkins
if [ ! -d /data ]
then
    echo Data folder does not exist. Creating it.
    mkdir /data
    chown $USER /data
fi

echo Packages and tools for O3DE setup complete
exit 0
