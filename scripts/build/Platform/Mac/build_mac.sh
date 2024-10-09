#!/usr/bin/env bash
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set -o errexit # exit on the first failure encountered

BASEDIR=$(dirname "$0")
source $BASEDIR/env_mac.sh

mkdir -p ${OUTPUT_DIRECTORY}
SOURCE_DIRECTORY=${PWD}
pushd $OUTPUT_DIRECTORY

LAST_CONFIGURE_CMD_FILE=ci_last_configure_cmd.txt
CONFIGURE_CMD="cmake '${SOURCE_DIRECTORY}' ${CMAKE_OPTIONS} ${EXTRA_CMAKE_OPTIONS}"
if [[ -n "$CMAKE_LY_PROJECTS" ]]; then
    CONFIGURE_CMD="${CONFIGURE_CMD} -DLY_PROJECTS='${CMAKE_LY_PROJECTS}'"
fi
if [[ ! -e "CMakeCache.txt" ]]; then
    echo [ci_build] First run, generating
    RUN_CONFIGURE=1
elif [[ ! -e ${LAST_CONFIGURE_CMD_FILE} ]]; then
    echo [ci_build] Last run command not found, generating
    RUN_CONFIGURE=1
else
    # Detect if the input has changed
    LAST_CMD=$(<${LAST_CONFIGURE_CMD_FILE})
    if [[ "${LAST_CMD}" != "${CONFIGURE_CMD}" ]]; then
        echo [ci_build] Last run command different, generating
        RUN_CONFIGURE=1
    fi
fi

# temporarily enabling cmake regeneration for this platform
# We have observed cases where continous integration has not regenerated but a regeneration was required, leaving the build in a bad state
RUN_CONFIGURE=1

if [[ ! -z "$RUN_CONFIGURE" ]]; then
    # have to use eval since $CMAKE_OPTIONS (${EXTRA_CMAKE_OPTIONS}) contains quotes that need to be processed
    eval echo [ci_build] ${CONFIGURE_CMD}
    eval ${CONFIGURE_CMD}
    # Save the run only if success
    echo "${CONFIGURE_CMD}" > ${LAST_CONFIGURE_CMD_FILE}
fi

# Split the configuration on semi-colon and use the cmake --build wrapper to run the underlying build command for each
for config in $(echo $CONFIGURATION | sed "s/;/ /g")
do
    echo [ci_build] cmake --build . --target ${CMAKE_TARGET} --config ${config} -j $(/usr/sbin/sysctl -n hw.ncpu) -- ${CMAKE_NATIVE_BUILD_ARGS}
    cmake --build . --target ${CMAKE_TARGET} --config ${config} -j $(/usr/sbin/sysctl -n hw.ncpu) -- ${CMAKE_NATIVE_BUILD_ARGS}
done

popd
