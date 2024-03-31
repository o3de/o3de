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


# Locate and make sure cmake is in the path
if [[ "$OSTYPE" = *"darwin"* ]];
then
    PAL=Mac
    ARCH=
else
    PAL=Linux
    ARCH=$( uname -m )
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

# Calculate the engine ID
CALC_PATH=$DIR/../cmake/CalculateEnginePathId.cmake
LY_ROOT_FOLDER=$DIR/..
ENGINE_ID=$(cmake -P $CALC_PATH $LY_ROOT_FOLDER)
if [ $? -ne 0 ]
then
    echo "Unable to calculate engine ID"
    exit 1
fi

# Set the expected location of the python venv for this engine and the locations of the critical scripts/executables 
# needed to run python within the venv properly
PYTHON_VENV=$HOME/.o3de/Python/venv/$ENGINE_ID
PYTHON_VENV_ACTIVATE=$PYTHON_VENV/bin/activate
PYTHON_VENV_PYTHON=$PYTHON_VENV/bin/python
if [ ! -f $PYTHON_VENV_PYTHON ]
then
    echo "Python has not been downloaded/configured yet."    
    echo "Try running $DIR/get_python.sh first."
    exit 1
fi

# Determine the current package from where the current venv was initiated from
PYTHON_VENV_HASH_FILE=$PYTHON_VENV/.hash
if [ ! -f $PYTHON_VENV_HASH_FILE ]
then
    echo "Python has not been downloaded/configured yet."
    echo "Try running $DIR/get_python.sh first."
    exit 1
fi
PYTHON_VENV_HASH=$(cat $PYTHON_VENV_HASH_FILE)

# Calculate the expected hash from the current python package
CURRENT_PYTHON_PACKAGE_HASH=$(cmake -P $DIR/get_python_package_hash.cmake $DIR/.. $PAL $ARCH)

if [ "$PYTHON_VENV_HASH" != "$CURRENT_PYTHON_PACKAGE_HASH" ]
then
    echo "Python has been updated since the last time the python command was invoked."
    echo "Run $DIR/get_python.sh to update."
    exit 1
fi

# Activate the venv environment
source $PYTHON_VENV_ACTIVATE

# Make sure that python shared library that is loaded by the python linked in the venv folder
# is the one that is loaded by injecting the shared lib path before invoking python. Otherwise,
# the shared library may not be found or it could load it from a different location.
PYTHON_LIB_PATH=$PYTHON_VENV/lib
PYTHONNOUSERSITE=1 LD_LIBRARY_PATH="$PYTHON_LIB_PATH:$LD_LIBRARY_PATH" PYTHONPATH= "$PYTHON_VENV_PYTHON" "$@"
exit $?
