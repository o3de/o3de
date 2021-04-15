#!/bin/bash 
set -euo pipefail

# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

JDK_VERSION="8.282.08.1"

echo "Installing Corretto JDK ${JDK_VERSION}"
curl -o /tmp/amazon-corretto-${JDK_VERSION}-macosx-x64.pkg https://corretto.aws/downloads/resources/${JDK_VERSION}/amazon-corretto-${JDK_VERSION}-macosx-x64.pkg
if sudo installer -allowUntrusted -pkg /tmp/amazon-corretto-${JDK_VERSION}-macosx-x64.pkg -target / ; then
    rm  /tmp/amazon-corretto-${JDK_VERSION}-macosx-x64.pkg
else
    echo "Installation failed!"
    exit 1
fi