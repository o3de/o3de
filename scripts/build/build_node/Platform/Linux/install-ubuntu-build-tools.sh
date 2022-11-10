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
# Add the kitware repository for cmake if necessary
#

KITWARE_REPO_COUNT=$(cat /etc/apt/sources.list | grep ^deb | grep https://apt.kitware.com/ubuntu/ | wc -l)

if [ $KITWARE_REPO_COUNT -eq 0 ]
then
    echo Adding Kitware Repository for the cmake

    wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | sudo tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null
    CMAKE_DEB_REPO="'deb https://apt.kitware.com/ubuntu/ $UBUNTU_DISTRO main'"

    # Add the appropriate kitware repository to apt
    if [ "$UBUNTU_DISTRO" == "bionic" ]
    then
        CMAKE_DISTRO_VERSION=3.20.1-0kitware1ubuntu18.04.1
        apt-add-repository 'deb https://apt.kitware.com/ubuntu/ bionic main'
    elif [ "$UBUNTU_DISTRO" == "focal" ]
    then
        CMAKE_DISTRO_VERSION=3.20.1-0kitware1ubuntu20.04.1
        apt-add-repository 'deb https://apt.kitware.com/ubuntu/ focal main'
    elif [ "$UBUNTU_DISTRO" == "jammy" ]
    then
        # Ubuntu 22.04 already has an acceptable version of cmake
        echo "Ubuntu 22.04's cmake package already at version 3.22.1"
    fi
    apt-get update
else
    echo  Kitware Repository repo already set
fi


#
# Add Amazon Corretto repository to install the necessary JDK for Jenkins and Android
#

CORRETTO_REPO_COUNT=$(cat /etc/apt/sources.list | grep ^dev | grep https://apt.corretto.aws | wc -l)

if [ $CORRETTO -eq 0 ]
then
    echo Adding Corretto Repository for JDK
    
    wget -O- https://apt.corretto.aws/corretto.key | apt-key add - 
    add-apt-repository 'deb https://apt.corretto.aws stable main'
    apt-get update
else
    echo Corretto repo already set
fi

# Read from the package list and process each package
PACKAGE_FILE_LIST=package-list.ubuntu-$UBUNTU_DISTRO.txt

echo Reading package list $PACKAGE_FILE_LIST

# Read each line (strip out comment tags)
for PREPROC_LINE in $(cat $PACKAGE_FILE_LIST | sed 's/#.*$//g')
do
    LINE=$(echo $PREPROC_LINE | tr -d '\r\n')
    PACKAGE=$(echo $LINE | awk -F / '{$1=$1;print $1}')
    if [ "$PACKAGE" != "" ]  # Skip blank lines
    then
        PACKAGE_VER=$(echo $LINE | awk -F / '{$2=$2;print $2}')
        if [ "$PACKAGE_VER" == "" ]
        then
            # Process non-versioned packages
            INSTALLED_COUNT=$(apt list --installed 2>/dev/null | grep ^$PACKAGE/ | wc -l)
            if [ $INSTALLED_COUNT -eq 0 ]
            then
                echo Installing $PACKAGE
                apt-get install $PACKAGE -y
            else
                INSTALLED_VERSION=$(apt list --installed 2>/dev/null | grep ^$PACKAGE/ | awk '{print $2}')
                echo $PACKAGE already installed \(version $INSTALLED_VERSION\)
            fi
        else
            # Process versioned packages
            INSTALLED_COUNT=$(apt list --installed 2>/dev/null | grep ^$PACKAGE/ | wc -l)
            if [ $INSTALLED_COUNT -eq 0 ]
            then
                echo Installing $PACKAGE \( $PACKAGE_VER \)
                apt-get install $PACKAGE=$PACKAGE_VER -y
            else
                INSTALLED_VERSION=$(apt list --installed 2>/dev/null | grep ^$PACKAGE/ | awk '{print $2}')
                if [ "$INSTALLED_VERSION" != "$PACKAGE_VER" ]
                then
                    echo $PACKAGE already installed but with the wrong version. Purging the package
                    apt purge --auto-remove $PACKAGE -y
                fi
                echo $PACKAGE already installed \(version $INSTALLED_VERSION\)
            fi
        fi
    fi

done

