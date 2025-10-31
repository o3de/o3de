#!/bin/bash

# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

SOURCE="${BASH_SOURCE[0]}"
# While $SOURCE is a symlink, resolve it
while [ -h "$SOURCE" ]; do
    DIR="$( cd -P "$( dirname "$SOURCE" )" && pwd )"
    SOURCE="$( readlink "$SOURCE" )"
    # If $SOURCE was a relative symlink (so no "/" as prefix, need to resolve it relative to the symlink base directory
    [[ $SOURCE != /* ]] && SOURCE="$DIR/$SOURCE"
done

DIR="$( cd -P "$( dirname "$SOURCE" )" && pwd )"
cd $DIR

install_dependencies () {
    echo installing via pip...
    $DIR/pip.sh install -r $DIR/requirements.txt --disable-pip-version-check --no-warn-script-location
    retVal=$?
    if [ $retVal -ne 0 ]; then
        echo "Failed to install the packages listed in $DIR/requirements.txt.  Check the log above!"
        return $retVal
    fi

    # If we're building a container app, create a package from o3de then install that to remove absolute paths to o3de scripts
    if [ "$O3DE_PACKAGE_TYPE" == "SNAP" ]; then
        pushd $DIR/../scripts/o3de/
        $DIR/python.sh setup.py sdist
        popd
    fi
    # If the dist package is detected (result of a built container app), run the install of the o3de script library from the 
    # dist package so that the egg-link file will not be created inside the o3de script folder
    if [ -f $DIR/../scripts/o3de/dist/o3de-1.0.0.tar.gz ]; then
        $DIR/pip.sh install $DIR/../scripts/o3de/dist/o3de-1.0.0.tar.gz --no-deps --disable-pip-version-check --no-cache
    else
        $DIR/pip.sh install -e $DIR/../scripts/o3de --no-deps --disable-pip-version-check  --no-warn-script-location
    fi

    retVal=$?
    if [ $retVal -ne 0 ]; then
        echo "Failed to install $DIR/../scripts/o3de into python.  Check the log above!"
        return $retVal
    fi
}

# Overall strategy: Run python with --version to see if its already there
# otherwise, use cmake and the new package system to download it.
# To find cmake, search the path for cmake.  If not found, try a fixed known location.

# the version number below is only used if cmake isn't already on your path.
# if you update this version number, remember to update the one(s) in the other platform
# files, as well as in scripts/build/...

./python.sh --version > /dev/null
python_exitcode=$?
if [ $python_exitcode == 0 ]; then
    echo get_python.sh: Python is already downloaded: $(./python.sh --version)
    install_dependencies
    exit $?
fi
if [[ "$OSTYPE" = *"darwin"* ]];
then
    PAL=Mac
    CMAKE_FOLDER_RELATIVE_TO_ROOT=CMake.app/Contents/bin
elif [[ "$OSTYPE" == "msys" ]]; then #git bash
    PAL=Windows
    CMAKE_FOLDER_RELATIVE_TO_ROOT=bin
    LINUX_HOST_ARCHITECTURE=""
else
    PAL=Linux
    CMAKE_FOLDER_RELATIVE_TO_ROOT=bin

    LINUX_HOST_ARCHITECTURE=$( uname -m )
    if [[ "$LINUX_HOST_ARCHITECTURE" == "aarch64" ]]; then
        PAL_ARCH="_aarch64"
    elif [[ "$LINUX_HOST_ARCHITECTURE" == "x86_64" ]]; then
        PAL_ARCH="_x86_64"
    else
        echo "Linux host architecture ${LINUX_HOST_ARCHITECTURE} not supported."
        exit 1
    fi
fi

if ! [ -x "$(command -v cmake)" ]; then
    if [ -z ${LY_CMAKE_PATH} ]; then
        echo "ERROR: Could not find cmake on the PATH and LY_CMAKE_PATH is not defined, cannot continue."
        echo "Please add cmake to your PATH, or define LY_CMAKE_PATH"
        exit 1
    fi

    export PATH=$LY_CMAKE_PATH:$PATH
    if ! [ -x "$(command -v cmake)" ]; then
        echo "ERROR: Could not find cmake on the PATH or at the known location: $LY_CMAKE_PATH"
        echo "Please add cmake to the environment PATH or place it at the above known location."
        exit 1
    fi
fi

echo Using cmake located at: $(which cmake)
echo $(cmake --version)

cd ..

if [ "$LY_3RDPARTY_PATH" == "" ]
then
    LY_3RDPARTY_PATH=$HOME/.o3de/3rdParty
fi
LY_ROOT_FOLDER=$DIR/..

cmake -DPAL_PLATFORM_NAME:string=$PAL -DLY_3RDPARTY_PATH:string=$LY_3RDPARTY_PATH -DLY_ROOT_FOLDER="$LY_ROOT_FOLDER" -DLY_HOST_ARCHITECTURE_NAME_EXTENSION=$PAL_ARCH -P $DIR/get_python.cmake

retVal=$?
if [ $retVal -ne 0 ]; then
    echo Unable to fetch python using cmake.  
    echo  - Is LY_PACKAGE_SERVER_URLS set?  
    echo  - Do you have permission to access the packages?
    exit $retVal
fi

install_dependencies
exit $?
