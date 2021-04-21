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
    AWS_CLI_VERSION=`aws --version | awk '{print $1}' | awk -F/ '{print $2}'`
    echo AWS CLI \(version $AWS_CLI_VERSION\) already installed
fi




