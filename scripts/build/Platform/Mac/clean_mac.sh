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

if [[ -n "$CLEAN_OUTPUT_DIRECTORY" ]]; then
    echo "[ci_build] CLEAN_OUTPUT_DIRECTORY option set"
    if [[ -d $OUTPUT_DIRECTORY ]]; then
        echo "[ci_build] Deleting \"${OUTPUT_DIRECTORY}\""
        rm -rf ${OUTPUT_DIRECTORY}
    fi
fi