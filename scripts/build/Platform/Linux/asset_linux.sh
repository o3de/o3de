#!/usr/bin/env bash
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set -o errexit # exit on the first failure encountered

if [[ ! -d $OUTPUT_DIRECTORY ]]; then
    echo [ci_build] Error: $OUTPUT_DIRECTORY was not found
    exit 1
fi
pushd $OUTPUT_DIRECTORY

if [[ ! -e $ASSET_PROCESSOR_BINARY ]]; then
    echo [ci_build] Error: $ASSET_PROCESSOR_BINARY was not found
    exit 1
fi

for project in $(echo $CMAKE_LY_PROJECTS | sed "s/;/ /g")
do
    echo [ci_build] ${ASSET_PROCESSOR_BINARY} $ASSET_PROCESSOR_OPTIONS --project-path=$project --platforms=$ASSET_PROCESSOR_PLATFORMS
    ${ASSET_PROCESSOR_BINARY} $ASSET_PROCESSOR_OPTIONS --project-path=$project --platforms=$ASSET_PROCESSOR_PLATFORMS
done

popd
