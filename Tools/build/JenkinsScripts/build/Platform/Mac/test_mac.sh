#!/usr/bin/env bash
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

set -o errexit # exit on the first failure encountered

BASEDIR=$(dirname "$0")
source $BASEDIR/env_mac.sh

if [[ ! -d $OUTPUT_DIRECTORY ]]; then
    echo [ci_build] Error: $OUTPUT_DIRECTORY was not found
    exit 1
fi
pushd $OUTPUT_DIRECTORY

# Find the CTEST_RUN_FLAGS from the CMakeCache.txt file, then replace the $<CONFIG> with the current configuration
IFS='=' read -ra CTEST_RUN_FLAGS <<< $(cmake -N -LA . | grep "CTEST_RUN_FLAGS:STRING")
CTEST_RUN_FLAGS=${CTEST_RUN_FLAGS[1]/$<CONFIG>/${CONFIGURATION}}

# Run ctest
echo [ci_build] ctest ${CTEST_RUN_FLAGS} ${CTEST_OPTIONS}
ctest ${CTEST_RUN_FLAGS} ${CTEST_OPTIONS}

popd