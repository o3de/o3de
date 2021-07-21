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
# Setup AWS CLI if needed
#
aws --version >/dev/null 2>&1
if [ $? -ne 0 ]
then
    echo Setting up AWS CLI
    curl "https://awscli.amazonaws.com/awscli-exe-linux-x86_64.zip" -o "awscliv2.zip"
    unzip awscliv2.zip
    ./aws/install
    rm -rf ./aws
else
    AWS_CLI_VERSION=$(aws --version | awk '{print $1}' | awk -F/ '{print $2}')
    echo AWS CLI \(version $AWS_CLI_VERSION\) already installed
fi




