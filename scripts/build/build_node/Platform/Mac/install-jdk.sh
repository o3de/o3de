#!/bin/bash 
set -euo pipefail

# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

JDK_VERSION="11.0.17.8.1"

echo "Installing Corretto JDK ${JDK_VERSION}"
curl -o /tmp/amazon-corretto-${JDK_VERSION}-macosx-x64.pkg https://corretto.aws/downloads/resources/${JDK_VERSION}/amazon-corretto-${JDK_VERSION}-macosx-x64.pkg
if sudo installer -allowUntrusted -pkg /tmp/amazon-corretto-${JDK_VERSION}-macosx-x64.pkg -target / ; then
    rm  /tmp/amazon-corretto-${JDK_VERSION}-macosx-x64.pkg
else
    echo "Installation failed!"
    exit 1
fi
