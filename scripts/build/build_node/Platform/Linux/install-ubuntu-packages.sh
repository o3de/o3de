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

# Read from the package list and process each package
PACKAGE_FILE_LIST=package-list.ubuntu-$(. /etc/os-release && echo $UBUNTU_CODENAME).txt

echo Reading package list $PACKAGE_FILE_LIST

apt-get update

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
