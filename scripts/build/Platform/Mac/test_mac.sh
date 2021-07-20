#!/usr/bin/env bash
#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
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