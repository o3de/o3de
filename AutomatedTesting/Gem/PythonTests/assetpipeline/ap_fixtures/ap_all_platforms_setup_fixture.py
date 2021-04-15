"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

Fixture for organizing directories associated with tests that involve assets for all platforms

"""

# Import builtin libraries
import pytest
import os
from typing import Dict

from . import ap_setup_fixture as ap_setup_fixture


@pytest.fixture
def ap_all_platforms_setup_fixture(request, workspace, ap_setup_fixture) -> Dict[str, str]:

    dev_dir = os.path.join(workspace.paths.engine_root())
    cache_dir = workspace.paths.cache()

    # add some useful locations
    resources = ap_setup_fixture

    # Current project's current platform's cache
    resources["platform_cache"] = os.path.join(workspace.paths.platform_cache(), workspace.project.lower())

    # Specific platform cache locations
    resources["pc_cache_location"] = os.path.join(cache_dir, "pc")
    resources["es3_cache_location"] = os.path.join(cache_dir, "es3")
    resources["ios_cache_location"] = os.path.join(cache_dir, "ios")
    resources["osx_gl_cache_location"] = os.path.join(cache_dir, "osx_gl")
    resources["provo_cache_location"] = os.path.join(cache_dir, "provo")
    resources["all_platforms"] = ["pc", "es3", "ios", "osx_gl", "provo"]

    return resources
