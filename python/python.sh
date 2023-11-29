#!/bin/bash

# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

SOURCE="${BASH_SOURCE[0]}"
# While $SOURCE is a symlink, resolve it
while [[ -h "$SOURCE" ]]; do
    DIR="$( cd -P "$( dirname "$SOURCE" )" && pwd )"
    SOURCE="$( readlink "$SOURCE" )"
    # If $SOURCE was a relative symlink (so no "/" as prefix, need to resolve it relative to the symlink base directory
    [[ $SOURCE != /* ]] && SOURCE="$DIR/$SOURCE"
done
DIR="$( cd -P "$( dirname "$SOURCE" )" && pwd )"


# Locate and make sure cmake is in the patch
if [[ "$OSTYPE" = *"darwin"* ]];
then
    PAL=Mac
    CMAKE_FOLDER_RELATIVE_TO_ROOT=CMake.app/Contents/bin
else
    PAL=Linux
    CMAKE_FOLDER_RELATIVE_TO_ROOT=bin
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

CALC_PATH=$DIR/../cmake/CalculateEnginePathId.cmake
LY_ROOT_FOLDER=$DIR/..
ENGINE_ID=$(cmake -P $CALC_PATH $LY_ROOT_FOLDER)

if [ "$LY_3RDPARTY_PATH" == "" ]
then
    LY_3RDPARTY_PATH=$HOME/.o3de/3rdParty
fi

# Set the expected location of the python venv for this engine and the locations of the critical scripts/executables 
# needed to run python within the venv properly
PYTHON_VENV=$LY_3RDPARTY_PATH/venv/$ENGINE_ID
PYTHON_VENV_ACTIVATE=$PYTHON_VENV/bin/activate
PYTHON_VENV_PYTHON=$PYTHON_VENV/bin/python
if [ ! -f $PYTHON_VENV_PYTHON ]
then
    echo "Python has not been downloaded/configured yet."    
    echo "Try running $DIR/get_python.sh first."
    exit /b 1
fi

PYTHONPATH=""

echo source $PYTHON_VENV_ACTIVATE
source $PYTHON_VENV_ACTIVATE

PYTHONNOUSERSITE=1 "$PYTHON_VENV_PYTHON" "$@"
exit $?
