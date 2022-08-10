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

if ! command -v cpack &> /dev/null; then
    echo "[ci_build] CPack not found"
    exit 1
fi

echo [ci_build] cpack --version
cpack --version

eval echo [ci_build] cpack -C ${CONFIGURATION} ${CPACK_OPTIONS}
eval cpack -C ${CONFIGURATION} ${CPACK_OPTIONS}

popd
