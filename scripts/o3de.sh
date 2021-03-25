#!/bin/sh

#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

#Use $(cd dirname $0) to get the absolute path of this scripts folder
#Note this does not actually change the working directory
SCRIPT_DIR=$(cd `dirname $0` && pwd)

#The engine root is always the parent of the scripts directory, so $(dirname "$SCRIPT_DIR") should get us engine root
ENGINE_ROOT=$(dirname "$SCRIPT_DIR")

#The engines python is in the engineroot/python
PYTHON_DIRECTORY="$ENGINE_ROOT/python"

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