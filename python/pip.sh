#!/bin/bash

# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
#
#
pushd $(dirname "$0")
./python.sh -m pip "$@"
PIP_EXIT=$?

popd
exit $PIP_EXIT
