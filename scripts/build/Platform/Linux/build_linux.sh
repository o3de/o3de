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
source $BASEDIR/env_linux.sh

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
if [[ ! -z "$RUN_CONFIGURE" ]]; then
    # have to use eval since $CMAKE_OPTIONS (${EXTRA_CMAKE_OPTIONS}) contains quotes that need to be processed
    eval echo [ci_build] ${CONFIGURE_CMD}
    eval ${CONFIGURE_CMD}
    # Save the run only if success
    echo "${CONFIGURE_CMD}" > ${LAST_CONFIGURE_CMD_FILE}
fi

TOTAL_MEMORY=$(cat /proc/meminfo | grep MemTotal | awk '{print $2}')
TOTAL_CORE_COUNT=$(grep -c processor /proc/cpuinfo)
echo "Total Memory        : $TOTAL_MEMORY kB"
echo "Total Cores         : $TOTAL_CORE_COUNT"

# For machines that has a core to memory ratio that could cause running out of resources, 
# we will check for an environment variable LY_MIN_MEMORY_PER_CORE to restrict the number
# of cores that we will allow.  If set, it will use this as the minimum anticipated memory
# that all cores will need in order to no fall below that number. 
#
# This is based on the value the total memory (in kB) divided by the LY_MIN_MEMORY_PER_CORE 
# (in kB, e.g. 2 GB = 2097152 kB)

if [[ "${LY_MIN_MEMORY_PER_CORE:-0}" -gt 0 ]]; then
    MIN_MEMORY_PER_CORE=${LY_MIN_MEMORY_PER_CORE}
    CALCULATED_MAX_CORE_USAGE=$(expr $TOTAL_MEMORY / $MIN_MEMORY_PER_CORE - 1)
    CORE_COUNT=$((CALCULATED_MAX_CORE_USAGE > TOTAL_CORE_COUNT ? TOTAL_CORE_COUNT :  CALCULATED_MAX_CORE_USAGE))
    echo "Max Usable Cores    : $CORE_COUNT"
else
    CORE_COUNT=$TOTAL_CORE_COUNT
fi

# Split the configuration on semi-colon and use the cmake --build wrapper to run the underlying build command for each
for config in $(echo $CONFIGURATION | sed "s/;/ /g")
do
    eval echo [ci_build] cmake --build . --target ${CMAKE_TARGET} --config ${config} -j $CORE_COUNT -- ${CMAKE_NATIVE_BUILD_ARGS}
    eval cmake --build . --target ${CMAKE_TARGET} --config ${config} -j $CORE_COUNT -- ${CMAKE_NATIVE_BUILD_ARGS}
done
popd
