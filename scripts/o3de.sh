#!/bin/sh

#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

#Use $(cd dirname $0) to get the absolute path of this scripts folder
#Note this does not actually change the working directory
SCRIPT_DIR=$(cd `dirname $0` && pwd)

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