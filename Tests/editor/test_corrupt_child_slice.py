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
C2603859: Parent Slice should still load on map if a child slice is corrupted.
https://testrail.agscollab.com/index.php?/cases/view/2603859
"""

import os
import pytest

# Bail on the test if ly_test_tools doesn't exist.
pytest.importorskip("ly_test_tools")

from Tests.ly_shared import hydra_lytt_test_utils as hydra_utils
import ly_test_tools.environment.file_system as file_system

test_directory = os.path.join(os.path.dirname(__file__), "EditorScripts")
log_monitor_timeout = 120


@pytest.mark.parametrize("platform", ["win_x64_vs2017"])
@pytest.mark.parametrize("configuration", ["profile"])
@pytest.mark.parametrize("project", ["SamplesProject"])
@pytest.mark.parametrize("spec", ["all"])
@pytest.mark.parametrize("level", ["tmp_level"])
class TestCorruptChildSlice(object):
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

    @pytest.mark.test_case_id("C2603859")
    def test_corrupt_child_slice(self, request, editor, level):
        expected_lines = [
            "Corrupt child slice test complete."
        ]
        unexpected_lines = [
            "Failed to find warning message in push dialog.",
            "Failed to find Invalid References warning message.",
            "Failed to find CorruptB invalid reference removal message.",
            "Failed to create new entity.",
            "Failed to set entity name.",
            "Failed to create slice.",
            "Failed to find the outliner.",
            "Failed to find the outliner main window.",
            "Failed to find the outliner tree view.",
            "Failed to find the console.",
            "Failed to find console textEdit.",
            "Failed to create level",
            "Failed to reload test_level_1",
            "Failed to reload test_level_2",
            "Failed to find error message dialog.",
            "No CorruptA entity found after reload",
            "No CorruptB entity found after reload",
            "CorruptC entity found after reload",
            "Failed to find push dialog.",
            "Failed to reload test_level_1",
            "Failed to reload test_level_2",
            "Level failed to load without errors.",
            "Error occurred instantiating CorruptA."
            "Test Failed."
        ]

        hydra_utils.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "corrupt_child_slice.py",
            expected_lines,
            unexpected_lines=unexpected_lines,
            auto_test_mode=False,
            run_python="--runpython",
            cfg_args=[level],
            timeout=log_monitor_timeout,
        )

