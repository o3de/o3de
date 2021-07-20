#!/bin/bash

# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

# This script must be run as root
if [[ $EUID -ne 0 ]]
then
    echo "This script must be run as root (sudo)"
    exit 1
fi

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

# Install the necessary build tools
./install-ubuntu-build-tools.sh
if [ $? -ne 0 ]
then
    echo Error installing ubuntu tools
    exit 1
fi


echo Packages and tools for O3DE setup complete
exit 0
