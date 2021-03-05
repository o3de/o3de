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

CURRENT=$PWD
cd "$PWD/.." || exit
BASE_PATH=$PWD
cd "$CURRENT" || exit
PYTHON_DIRECTORY="${BASE_PATH}/python"
if [ ! -d "$PYTHON_DIRECTORY" ]; then
  echo "Python dir not found: $PYTHON_DIRECTORY"
  exit 1
fi
PYTHON_EXECUTABLE="$PYTHON_DIRECTORY/python.sh"
if [ ! -f "$PYTHON_EXECUTABLE" ]; then
  echo "Python executable not found: $PYTHON_EXECUTABLE"
  exit 1
fi
$PYTHON_EXECUTABLE "$BASE_PATH/scripts/lmbr.py" $*
cd "$CURRENT" || exit

