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

XCODE_VER="11.4"
S3_BUCKET=$1

echo "Installing Xcode ${XCODE_VER}"
aws s3 cp ${S3_BUCKET}/Xcode$XCODE_VER.xip /tmp/
cd /Applications
 
if sudo xip -x /tmp/Xcode$XCODE_VER.xip ; then
    sudo xcodebuild -license accept
    rm /tmp/Xcode${XCODE_VER}.xip
else
    echo "Installation failed!"
    exit 1
fi

echo "Installing Xcode Command line tools ${XCODE_VER}"
aws s3 cp ${S3_BUCKET}/Command_Line_Tools_for_${XCODE_VER}.dmg /tmp/
sudo hdiutil attach /tmp/Command_Line_Tools_for_${XCODE_VER}.dmg

if sudo installer -pkg /Volumes/Command\ Line\ Developer\ Tools/Command\ Line\ Tools.pkg -target / ; then
    sudo hdiutil detach /Volumes/Command\ Line\ Developer\ Tools/
    rm /tmp/Command_Line_Tools_for_${XCODE_VER}.dmg
else
    echo "Installation failed!" 
    sudo hdiutil detach /Volumes/Command\ Line\ Developer\ Tools/
    exit 1
fi

echo "Installing XCode iOS packages"
sudo installer -pkg /Applications/Xcode.app/Contents/Resources/Packages/MobileDevice.pkg -target /
sudo installer -pkg /Applications/Xcode.app/Contents/Resources/Packages/MobileDeviceDevelopment.pkg -target /

echo "Enabling developer mode"
sudo /usr/sbin/DevToolsSecurity --enable