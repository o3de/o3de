#!/bin/bash
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

# Clone O3DE and bootstrap
echo "Cloning o3de"
git clone --single-branch -b $O3DE_BRANCH $O3DE_REPO $O3DE_ROOT && \
    git -C $O3DE_ROOT lfs install && \
    git -C $O3DE_ROOT lfs pull && \
    git -C $O3DE_ROOT reset --hard $O3DE_COMMIT 
if [ $? -ne 0 ]
then
    echo "Error cloning o3de from $O3DE_REPO"
    exit 1
fi

$O3DE_ROOT/python/get_python.sh 
if [ $? -ne 0 ]
then
    echo "Error bootstrapping O3DE from $O3DE_REPO"
    exit 1
fi

# Build the installer package
export O3DE_BUILD=$WORKSPACE/build
export CONFIGURATION=profile
export OUTPUT_DIRECTORY=$O3DE_BUILD
export O3DE_PACKAGE_TYPE=DEB
export CMAKE_OPTIONS="-G 'Ninja Multi-Config' -DLY_PARALLEL_LINK_JOBS=4 -DLY_DISABLE_TEST_MODULES=TRUE -DO3DE_INSTALL_ENGINE_NAME=o3de-sdk -DLY_STRIP_DEBUG_SYMBOLS=TRUE"
export EXTRA_CMAKE_OPTIONS="-DLY_INSTALLER_AUTO_GEN_TAG=TRUE -DLY_INSTALLER_DOWNLOAD_URL=${INSTALLER_DOWNLOAD_URL} -DLY_INSTALLER_LICENSE_URL=${INSTALLER_DOWNLOAD_URL}/license -DO3DE_INCLUDE_INSTALL_IN_PACKAGE=TRUE"
export CPACK_OPTIONS="-D CPACK_UPLOAD_URL=${CPACK_UPLOAD_URL}"
export CMAKE_TARGET=all

cd $O3DE_ROOT && \
    $O3DE_ROOT/scripts/build/Platform/Linux/build_installer_linux.sh
if [ $? -ne 0 ]
then
    echo "Error building installer"
    exit 1
fi

# Install from the installer package
su $O3DE_USER -c "sudo dpkg -i $O3DE_BUILD/CPackUploads/o3de_4.2.0.deb"
if [ $? -ne 0 ]
then
    echo "Error installing O3DE for the o3de user"
    exit 1
fi

# Cleanup the source from github and all intermediate files
rm -rf $O3DE_ROOT
rm -rf $O3DE_BUILD
rm -rf $HOME/.o3de
rm -rf $HOME/O3DE
rm -rf /home/o3de/.o3de
rm -rf /home/o3de/O3DE

# The python libraries that are linked in the engine needs to be writable during python bootstrapping for the O3DE user from the installer
chmod -R a+w /opt/O3DE/$(ls /opt/O3DE/)/Tools/LyTestTools \
  && chmod -R a+w /opt/O3DE/$(ls /opt/O3DE/)/Tools/RemoteConsole/ly_remote_console \
  && chmod -R a+w /opt/O3DE/$(ls /opt/O3DE/)/Gems/Atom/RPI/Tools \
  && chmod -R a+w /opt/O3DE/$(ls /opt/O3DE/)/Gems/AtomLyIntegration/TechnicalArt/DccScriptingInterface \
  && chmod -R a+w /opt/O3DE/$(ls /opt/O3DE/)/scripts/o3de
if [ $? -ne 0 ]
then
    echo "Error making O3DE python library folders writeable."
    exit 1
fi

exit 0
