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
C6130788: Recover a layer
https://testrail.agscollab.com/index.php?/cases/view/6130788
"""

import os
import pytest

# Bail on the test if ly_test_tools doesn't exist.
pytest.importorskip("ly_test_tools")

from Tests.hydra.hydra_utils import launch_test_case
from Tests.ly_shared import hydra_lytt_test_utils as hydra_utils
import ly_test_tools.environment.file_system as file_system

test_directory = os.path.join(os.path.dirname(__file__), "EditorScripts")
log_monitor_timeout = 60


@pytest.mark.parametrize("platform", ["win_x64_vs2017"])
@pytest.mark.parametrize("configuration", ["profile"])
@pytest.mark.parametrize("project", ["SamplesProject"])
@pytest.mark.parametrize("spec", ["all"])
@pytest.mark.parametrize("level", ["tmp_level"])
class TestRecoverLayer(object):
    @pytest.fixture(autouse=True)
    def setup_teardown(self, request, workspace, editor, project, level):
        def teardown():
            workspace = editor.workspace
            # delete temp level
            file_system.delete([os.path.join(workspace.paths.dev(), project, "Levels", "test_level_1")], True, True)
            file_system.delete([os.path.join(workspace.paths.dev(), project, "Levels", "test_level_2")], True, True)

        # Setup - add the teardown finalizer
        request.addfinalizer(teardown)
        file_system.delete([os.path.join(workspace.paths.dev(), project, "Levels", "test_level_1")], True, True)
        file_system.delete([os.path.join(workspace.paths.dev(), project, "Levels", "test_level_2")], True, True)

    @pytest.mark.test_case_id("C6130788")
    def test_recover_layer(self, request, editor, level):
        expected_lines = [
            "Saved level 2 with layers",
            "Saved level 2 without layers",
            "Reopened level 1",
            "Reopened level 2",
            "Recover Layer test complete."
        ]
        unexpected_lines = [
            "Failed to find Layer1 after recovery",
            "Failed to find Layer2 after recovery",
            "Failed to find inspector window",
            "Failed to create level",
            "Layer1 creation failed.",
            "Layer2 creation failed.",
            "Failed to reload test_level_1",
            "Layer1 save not set to binary.",
            "Layer2 save set to binary.",
            "Failed to undo layer recovery.",
            "Failed to redo layer recovery."
            "Test Failed."
        ]

        hydra_utils.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "recover_layer.py",
            expected_lines,
            unexpected_lines=unexpected_lines,
            auto_test_mode=False,
            run_python="--runpython",
            cfg_args=[level],
            timeout=log_monitor_timeout,
        )

