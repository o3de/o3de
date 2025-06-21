#!/bin/bash

#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# This script must search for the o3de python script in a sibling python
# directory.  In order to do this, it needs to de-symlink itself to find out
# where it is truly located.  (deb installers symlink it to a bin folder).
SOURCE="${BASH_SOURCE[0]}"
# While $SOURCE is a symlink, resolve it
while [[ -h "$SOURCE" ]]; do
    DIR="$( cd -P "$( dirname "$SOURCE" )" && pwd )"
    SOURCE="$( readlink "$SOURCE" )"
    # If $SOURCE was a relative symlink (so no "/" as prefix, need to resolve it relative to the symlink base directory
    [[ $SOURCE != /* ]] && SOURCE="$DIR/$SOURCE"
done
SCRIPT_DIR="$( cd -P "$( dirname "$SOURCE" )" && pwd )"

#python should be in the base path
BASE_PATH=$(dirname "$SCRIPT_DIR")
PYTHON_DIRECTORY="$BASE_PATH/python"

#If engine python exists use it, if not try the system python
if [ ! -d "$PYTHON_DIRECTORY" ]; then
  PYTHON_EXECUTABLE="python"
else
  PYTHON_EXECUTABLE="$PYTHON_DIRECTORY/python.sh"
fi
if [ ! -f "$PYTHON_EXECUTABLE" ]; then
  echo "Python executable not found: $PYTHON_EXECUTABLE"
  exit 1
fi

#run the o3de.py pass along the command
$PYTHON_EXECUTABLE "$SCRIPT_DIR/o3de.py" $*
exit $?