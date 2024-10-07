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
# Install the necessary tools to download, install, and run the android tools
#
apt install -y openjdk-11-jdk unzip wget

mkdir /opt/android-sdk-linux
wget -q https://dl.google.com/android/repository/commandlinetools-linux-6609375_latest.zip -O /tmp/commandlinetools-linux.zip
if [ $? -ne 0 ]
then
    echo "Error downloading the Android command line tools"
    exit 1
fi

unzip /tmp/commandlinetools-linux.zip -d /opt/android-sdk-linux/cmdline-tools
if [ $? -ne 0 ]
then
    echo "Error extracting the Android command line tools to /opt/android-sdk-linux/cmdline-tools"
    exit 1
fi
rm -f /tmp/commandlinetools-linux.zip

for i in "cmdline-tools;latest" "build-tools;30.0.2" "platform-tools" "platforms;android-24" "ndk;25.2.9519653"
do
    echo "Installing official $i"
    yes | /opt/android-sdk-linux/cmdline-tools/tools/bin/sdkmanager --install "$i"
    if [ $? -ne 0 ]
    then
        echo "Error installing '$i'"
        exit 1
    fi
done

# Add the path and necessary environment variable to build android based on NDK 25.2.9519653
echo 'export PATH="$PATH:/opt/android-sdk-linux/cmdline-tools/latest/bin/"' > /etc/profile.d/android-env.sh
echo "export LY_NDK_DIR=/opt/android-sdk-linux/ndk/25.2.9519653" >> /etc/profile.d/android-env.sh
chmod a+x /etc/profile.d/android-env.sh

