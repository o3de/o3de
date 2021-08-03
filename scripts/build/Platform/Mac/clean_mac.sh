#!/usr/bin/env bash
#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set -o errexit # exit on the first failure encountered

# Jenkins defines environment variables for parameters and passes "false" to variables 
# that are not set. Here we clear them if they are false so we can also just define them
# from command line
if [[ "${CLEAN_ASSETS}" == "false" ]]; then
    CLEAN_ASSETS=
fi
if [[ "${CLEAN_OUTPUT_DIRECTORY}" == "false" ]]; then
    CLEAN_OUTPUT_DIRECTORY=
fi

if [[ -n "$CLEAN_ASSETS" ]]; then
    echo "[ci_build] CLEAN_ASSETS option set"
    for project in $(echo $CMAKE_LY_PROJECTS | sed "s/;/ /g")
    do
        if [[ -d "$project/Cache" ]]; then
            echo "[ci_build] Deleting \"$project/Cache\""
            rm -rf $project/Cache
        fi
    done
fi

# If the node label changes, we issue a clean output since node changes can change SDK/CMake/toolchains/etc
LAST_CONFIGURE_NODE_LABEL_FILE=ci_last_node_label.txt
if [[ -n "$NODE_LABEL" ]]; then
    if [[ -d $OUTPUT_DIRECTORY ]]; then
        pushd $OUTPUT_DIRECTORY
        if [[ -e ${LAST_CONFIGURE_NODE_LABEL_FILE} ]]; then
            LAST_NODE_LABEL=$(<${LAST_CONFIGURE_NODE_LABEL_FILE})    
        else
            LAST_NODE_LABEL=
        fi
        # Detect if the node label has changed
        if [[ "${LAST_NODE_LABEL}" != "${NODE_LABEL}" ]]; then
            echo [ci_build] Last run was done with node label \"${LAST_NODE_LABEL}\", new node label is \"${NODE_LABEL}\", forcing CLEAN_OUTPUT_DIRECTORY
            CLEAN_OUTPUT_DIRECTORY=1
        fi
        popd
    fi
fi

if [[ -n "$CLEAN_OUTPUT_DIRECTORY" ]]; then
    echo "[ci_build] CLEAN_OUTPUT_DIRECTORY option set"
    if [[ -d $OUTPUT_DIRECTORY ]]; then
        echo "[ci_build] Deleting \"${OUTPUT_DIRECTORY}\""
        rm -rf ${OUTPUT_DIRECTORY}
    fi
fi

mkdir -p ${OUTPUT_DIRECTORY}
# Save the node label
pushd $OUTPUT_DIRECTORY
echo "${NODE_LABEL}" > ${LAST_CONFIGURE_NODE_LABEL_FILE}
popd
