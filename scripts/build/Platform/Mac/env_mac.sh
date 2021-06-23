#!/usr/bin/env bash
#
# Copyright (c) Contributors to the Open 3D Engine Project
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set -o errexit # exit on the first failure encountered

if ! command -v cmake &> /dev/null; then
    echo "[ci_build] CMake not found"
    exit 1
fi
