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

if ! command -v cmake &> /dev/null; then
    if [[ -z $LY_CMAKE_PATH ]]; then LY_CMAKE_PATH=${LY_3RDPARTY_PATH}/CMake/3.19.1/Linux/bin; fi
    if [[ ! -d $LY_CMAKE_PATH ]]; then
        echo "[ci_build] CMake path not found"
        exit 1
    fi
    PATH=${LY_CMAKE_PATH}:${PATH}
    if ! command -v cmake &> /dev/null; then
        echo "[ci_build] CMake not found"
        exit 1
    fi
fi

if ! command -v ninja &> /dev/null; then
    if [[ -z $LY_NINJA_PATH ]]; then LY_NINJA_PATH=${LY_3RDPARTY_PATH}/ninja/1.10.1/Linux; fi
    if [[ ! -d $LY_NINJA_PATH ]]; then
        echo "[ci_build] Ninja path not found"
        exit 1
    fi
    PATH=${LY_NINJA_PATH}:${PATH}
    if ! command -v ninja &> /dev/null; then
        echo "[ci_build] Ninja not found"
        exit 1
    fi
fi
