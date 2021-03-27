"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

Fixture for clearing "testingAssets" directory from <build>/dev/<project>
"""

# Import builtin libraries
import os
import pytest

# Import LyTestTools
import ly_test_tools.environment.file_system as fs

# Import fixtures
from ..ap_fixtures.ap_setup_fixture import ap_setup_fixture as ap_setup_fixture


@pytest.fixture
def clear_testingAssets_dir(request, workspace, ap_setup_fixture) -> None:
    env = ap_setup_fixture
    testingAssets_dir = os.path.join(env["project_dir"], "testingAssets")

    def remove_testingAssets_dir():
        fs.delete([testingAssets_dir], False, True)

    # Delete the directory upon execution
    remove_testingAssets_dir()

    # Delete the directory on teardown
    request.addfinalizer(remove_testingAssets_dir)
