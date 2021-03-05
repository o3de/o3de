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
#

# Extract an optional external engine path if present, otherwise use the cwd as the engine dir
EXTERNAL_ENGINE_PATH=`cat engine.json | grep "ExternalEnginePath" | awk -F":" '{ print $2 }' | sed "s/,//g" | sed "s/\"//g" | xargs echo -n`
if [ -z $EXTERNAL_ENGINE_PATH ]; then
    ENGINE_DIR=$(dirname "$0")
elif [ -d $EXTERNAL_ENGINE_PATH ]; then
    ENGINE_DIR=$EXTERNAL_ENGINE_PATH
else
    echo External Path in engine.json "$EXTERNAL_ENGINE_PATH" does not exist
    exit 1
fi

PYTHON_DIR=$ENGINE_DIR/python
PYTHON=$PYTHON_DIR/python.sh

if [ -e $PYTHON ]; then
    RUN_TESTS_DIR=$ENGINE_DIR/Tools/build/JenkinsScripts/build/runtests
    "$PYTHON" $RUN_TESTS_DIR/run_tests.py -d $ENGINE_DIR -p darwin_x64 -t Tests/pipeline/asset_bundler_tests  -vv -s --timeout 1000 --ignore=$ENGINE_DIR/*
else
    echo Could not find python.sh in "$PYTHON_DIR"
    exit 1
fi
