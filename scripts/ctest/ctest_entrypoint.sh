#!/bin/bash

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
# Continuous Integration CLI entrypoint script to start CTest, triggering post-build tests
#

CURRENT_SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE:-0}" )" >/dev/null 2>&1 && pwd )"
DEV_DIR="$( dirname "$CURRENT_SCRIPT_DIR" )"
PYTHON=$DEV_DIR/python/python.sh
CTEST_SCRIPT=$CURRENT_SCRIPT_DIR/ctest_driver.py

# pass all args to python-based CTest script
echo "Invoking: $PYTHON $CTEST_SCRIPT $*"
$PYTHON $CTEST_SCRIPT $*
