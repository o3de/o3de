#!/bin/bash 
set -euo pipefail

# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

PY3_VERSION="3.7.10"

echo "Installing Python ${PY3_VERSION}"
brew install python@${PY3_VERSION} || echo "Installation failed!"; exit 1
cat ../common/requirements.txt | cut -f1 -d"#" | sed '/^\s*$/d' | xargs -n 1 pip3 install