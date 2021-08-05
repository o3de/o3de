"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

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
    resources["android_cache_location"] = os.path.join(cache_dir, "android")
    resources["ios_cache_location"] = os.path.join(cache_dir, "ios")
    resources["mac_cache_location"] = os.path.join(cache_dir, "mac")
    resources["provo_cache_location"] = os.path.join(cache_dir, "provo")
    resources["all_platforms"] = ["pc", "android", "ios", "mac", "provo"]

    return resources
