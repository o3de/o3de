"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

"""
C2669525: Adding and deleting entities to a slice are able to be quick pushed to slice
https://testrail.agscollab.com/index.php?/cases/view/2669525
"""

import os
import pytest

# Bail on the test if ly_test_tools doesn't exist.
pytest.importorskip("ly_test_tools")

from Tests.ly_shared import hydra_lytt_test_utils as hydra_utils
import ly_test_tools.environment.file_system as file_system

test_directory = os.path.join(os.path.dirname(__file__), "EditorScripts")
log_monitor_timeout = 80


@pytest.mark.parametrize("platform", ["win_x64_vs2017"])
@pytest.mark.parametrize("configuration", ["profile"])
@pytest.mark.parametrize("project", ["SamplesProject"])
@pytest.mark.parametrize("spec", ["all"])
@pytest.mark.parametrize("level", ["tmp_level"])
class TestQuickPushSlice(object):
    @pytest.fixture(autouse=True)
    def setup_teardown(self, request, workspace, editor, project, level):
        def teardown():
            workspace = editor.workspace
            # delete temp level
            file_system.delete([os.path.join(workspace.paths.dev(), project, "Levels", level)], True, True)
            file_system.delete([os.path.join(workspace.paths.dev(), project, "test_slice.slice")], True, True)

        # Setup - add the teardown finalizer
        request.addfinalizer(teardown)
        file_system.delete([os.path.join(workspace.paths.dev(), project, "Levels", level)], True, True)
        file_system.delete([os.path.join(workspace.paths.dev(), project, "test_slice.slice")], True, True)

    @pytest.mark.test_case_id("C2669525")
    def test_quick_push_slice(self, request, editor, level):
        expected_lines = [
            "Slice Created: True",
            "Newly created child is updated in the instantiated slice",
            "Child deletion is updated in the instantiated slice",
        ]

        hydra_utils.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "quick_push_slice.py",
            expected_lines,
            cfg_args=[level],
            run_python="--runpython",
            timeout=log_monitor_timeout,
        )
