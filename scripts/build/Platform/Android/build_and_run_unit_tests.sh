#!/usr/bin/env bash
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set -o errexit # exit on the first failure encountered

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Run the gradle build first
source "$SCRIPT_DIR/gradle_linux.sh"

if [[ ! -d "$LY_3RDPARTY_PATH" ]]; then
    echo "[ci_build] LY_3RDPARTY_PATH is invalid or not set"
    exit 1
fi

if [[ -z "$ANDROID_SDK_ROOT" ]] && [[ -n "$ANDROID_HOME" ]]; then
    export ANDROID_SDK_ROOT="$ANDROID_HOME"
fi

if [[ ! -d "$ANDROID_SDK_ROOT" ]]; then
    echo "[ci_build] FAIL: ANDROID_SDK_ROOT=$ANDROID_SDK_ROOT"
    exit 1
fi
echo "ANDROID_SDK_ROOT=$ANDROID_SDK_ROOT"

PYTHON=python/python.sh
echo "[ci_build] $PYTHON scripts/build/Platform/Android/run_test_on_android_simulator.py --android-sdk-path $ANDROID_SDK_ROOT --build-path $OUTPUT_DIRECTORY --build-config $CONFIGURATION"
$PYTHON scripts/build/Platform/Android/run_test_on_android_simulator.py --android-sdk-path "$ANDROID_SDK_ROOT" --build-path "$OUTPUT_DIRECTORY" --build-config "$CONFIGURATION"

exit 0
