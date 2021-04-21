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
# Install curl if its not installed
#
curl --version >/dev/null 2>&1
if [ $? -ne 0 ]
then
    echo "Installing curl"
    apt-get install curl -y
fi


#
# If the linux distro is 20.04 (focal), we need libffi.so.6, which is not part of the focal distro. We 
# will install it from the bionic distro manually into focal. This is needed since Ubuntu 20.04 supports
# python 3.8 out of the box, but we are using 3.7
#
LIBFFI6_COUNT=`apt list --installed 2>/dev/null | grep libffi6 | wc -l`
if [ "$UBUNTU_DISTRO" == "focal" ] && [ $LIBFFI6_COUNT -eq 0 ]
then
    echo "Installing libffi for Ubuntu 20.04"

    pushd /tmp >/dev/null

    LIBFFI_PACKAGE_NAME=libffi6_3.2.1-8_amd64.deb
    LIBFFI_PACKAGE_URL=http://mirrors.kernel.org/ubuntu/pool/main/libf/libffi/

    curl --location $LIBFFI_PACKAGE_URL/$LIBFFI_PACKAGE_NAME -o $LIBFFI_PACKAGE_NAME
    if [ $? -ne 0 ]
    then
        echo Unable to download $LIBFFI_PACKAGE_URL/$LIBFFI_PACKAGE_NAME
        popd
        exit 1
    fi

    apt install ./$LIBFFI_PACKAGE_NAME -y
    if [ $? -ne 0 ]
    then
        echo Unable to install $LIBFFI_PACKAGE_NAME
        rm -f ./$LIBFFI_PACKAGE_NAME
        popd
        exit 1
    fi

    rm -f ./$LIBFFI_PACKAGE_NAME
    popd
    echo "libffi.so.6 installed"
fi

# Install the required build packages
apt-get install clang-6.0 -y            # For the compiler and its dependencies
apt-get install libglu1-mesa-dev -y     # For Qt (GL dependency)

# The following packages resolves a runtime error with Qt Plugins
apt-get install libxcb-xinerama0 -y     # For Qt plugins at runtime
apt-get install libxcb-xinput0 -y       # For Qt plugins at runtime

apt-get install libcurl4-openssl-dev -y # For HttpRequestor
apt-get install libsdl2-dev -y          # For WWise

apt-get install libz-dev -y
apt-get install mesa-common-dev -y

echo Build Libraries Setup Complete
