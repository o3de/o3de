#!/bin/bash 
set -euo pipefail

# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

PY3_VERSION="3.10.13"

echo "Installing Python ${PY3_VERSION}"
brew install python@${PY3_VERSION} || echo "Installation failed!"; exit 1
cat ../common/requirements.txt | cut -f1 -d"#" | sed '/^\s*$/d' | xargs -n 1 pip3 install