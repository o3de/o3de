"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

Fixture for clearing out 'MoveOutput' folders from \dev and \dev\PROJECT
"""

# Import builtin libraries
import pytest

# Import ly_shared
import ly_test_tools.lumberyard.pipeline_utils as pipeline_utils


@pytest.fixture
def clear_moveoutput_fixture(request, workspace) -> None:
    pipeline_utils.delete_MoveOutput_folders([workspace.paths.dev(), workspace.paths.project()])

    def teardown():
        pipeline_utils.delete_MoveOutput_folders([workspace.paths.dev(), workspace.paths.project()])

    request.addfinalizer(teardown)
