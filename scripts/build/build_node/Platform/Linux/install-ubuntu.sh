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
