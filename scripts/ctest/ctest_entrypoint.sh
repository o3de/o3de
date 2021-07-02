#!/bin/bash

#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# Continuous Integration CLI entrypoint script to start CTest, triggering post-build tests
#

CURRENT_SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE:-0}" )" >/dev/null 2>&1 && pwd )"
DEV_DIR=$( dirname "$( dirname "$CURRENT_SCRIPT_DIR" )" )
PYTHON=$DEV_DIR/python/python.sh
CTEST_SCRIPT=$CURRENT_SCRIPT_DIR/ctest_driver.py

# pass all args to python-based CTest script
echo "Invoking: $PYTHON $CTEST_SCRIPT $*"
$PYTHON $CTEST_SCRIPT $*
