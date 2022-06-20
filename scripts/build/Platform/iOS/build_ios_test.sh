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
source $BASEDIR/../Mac/env_mac.sh

mkdir -p ${OUTPUT_DIRECTORY}
SOURCE_DIRECTORY=${PWD}


ECHO Configuring for iOS Testing

LAST_CONFIGURE_CMD_FILE=ci_last_configure_cmd.txt
CONFIGURE_CMD="cmake -B ${OUTPUT_DIRECTORY} ${SOURCE_DIRECTORY} ${CMAKE_OPTIONS} ${EXTRA_CMAKE_OPTIONS}"
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
    echo [ci_build] ${CONFIGURE_CMD}
    eval ${CONFIGURE_CMD}
    # Save the run only if success
    echo "${CONFIGURE_CMD}" > ${LAST_CONFIGURE_CMD_FILE}
fi

if [ $? -ne 0 ]
then
    echo "CMake configuration failed"
    exit 1
fi

echo Building for iOS Testing
echo
echo [ci+build] ${SOURCE_DIRECTORY}/python/python.sh ${SOURCE_DIRECTORY}/cmake/Tools/Platform/iOS/build_ios_test.py -b ${OUTPUT_DIRECTORY} -c ${CONFIGURATION}
${SOURCE_DIRECTORY}/python/python.sh ${SOURCE_DIRECTORY}/cmake/Tools/Platform/iOS/build_ios_test.py -b ${OUTPUT_DIRECTORY} -c ${CONFIGURATION} 
if [ $? -ne 0 ]
then
    echo "iOS Test Build failed"
    exit 1
fi

ECHO Launching iOS Test
echo [ci+build] ${SOURCE_DIRECTORY}/python/python.sh ${SOURCE_DIRECTORY}/cmake/Tools/Platform/iOS/launch_ios_test.py -b ${OUTPUT_DIRECTORY} --device-name ${TARGET_DEVICE_NAME} --timeout ${TEST_MODULE_TIMEOUT}
${SOURCE_DIRECTORY}/python/python.sh ${SOURCE_DIRECTORY}/cmake/Tools/Platform/iOS/launch_ios_test.py -b ${OUTPUT_DIRECTORY} --device-name ${TARGET_DEVICE_NAME} --timeout ${TEST_MODULE_TIMEOUT}

if [ $? -ne 0 ]
then
    echo "iOS Test failed"
    exit 1
fi

echo "iOS Tests passed"
exit 0

