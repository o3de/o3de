#!/bin/bash 
set -euo pipefail

# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

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

echo "Installing cmake"
brew install cmake@3.22.1
