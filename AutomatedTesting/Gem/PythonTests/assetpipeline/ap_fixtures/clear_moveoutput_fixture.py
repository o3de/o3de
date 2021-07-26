"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Fixture for clearing out 'MoveOutput' folders from \dev and \dev\PROJECT
"""

# Import builtin libraries
import pytest

# Import ly_shared
import ly_test_tools.o3de.pipeline_utils as pipeline_utils


@pytest.fixture
def clear_moveoutput_fixture(request, workspace) -> None:
    pipeline_utils.delete_MoveOutput_folders([workspace.paths.engine_root(), workspace.paths.project()])

    def teardown():
        pipeline_utils.delete_MoveOutput_folders([workspace.paths.engine_root(), workspace.paths.project()])

    request.addfinalizer(teardown)
