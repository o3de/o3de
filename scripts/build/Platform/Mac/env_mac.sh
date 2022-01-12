#!/usr/bin/env bash
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set -o errexit # exit on the first failure encountered

if ! command -v cmake &> /dev/null; then
    echo "[ci_build] CMake not found"
    exit 1
fi

if [[ -n "${COMMAND_CWD}" ]]; then
    echo $(eval echo [ci_build] Changing CWD to $COMMAND_CWD)
    cd $(eval echo ${COMMAND_CWD})
fi
