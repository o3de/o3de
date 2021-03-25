"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

C16780783: Base Edit Menu Options (New Viewport Interaction Model)
https://testrail.agscollab.com/index.php?/cases/view/16780783
"""

import os
import pytest

# Bail on the test if ly_test_tools doesn't exist.
pytest.importorskip("ly_test_tools")

from Tests.ly_shared import hydra_lytt_test_utils as hydra_utils
import ly_test_tools.environment.file_system as file_system

test_directory = os.path.join(os.path.dirname(__file__), "EditorScripts")
log_monitor_timeout = 160


@pytest.mark.parametrize('project', ['AutomatedTesting'])
@pytest.mark.parametrize('level', ['tmp_level'])
class TestEditMenu(object):
    @pytest.fixture(autouse=True)
    def setup_teardown(self, request, workspace, editor, project, level):
        def teardown():
            workspace = editor.workspace
            # delete temp level
            file_system.delete([os.path.join(workspace.paths.dev(), project, "Levels", level)], True, True)

        # Setup - add the teardown finalizer
        request.addfinalizer(teardown)
        file_system.delete([os.path.join(workspace.paths.dev(), project, "Levels", level)], True, True)

    @pytest.mark.test_case_id("C16780783", "C2174438")
    def test_edit_menu_new_viewport(self, request, editor, level):
        expected_lines = [
            "Undo Action triggered",
            "Redo Action triggered",
            "Duplicate Action triggered",
            "Delete Action triggered",
            "Select All Action triggered",
            "Invert Selection Action triggered",
            "Toggle Pivot Location Action triggered",
            "Reset Entity Transform",
            "Reset Manipulator",
            "Snap angle Action triggered",
            "Move Action triggered",
            "Rotate Action triggered",
            "Scale Action triggered",
            "Lock Selection Action triggered",
            "Unlock All Entities Action triggered",
            "Global Preferences Action triggered",
            "Graphics Settings Action triggered",
            "Editor Settings Manager Action triggered",
            "Very High Action triggered",
            "High Action triggered",
            "Medium Action triggered",
            "Low Action triggered",
            "Apple TV Action triggered",
            "Customize Keyboard Action triggered",
            "Export Keyboard Settings Action triggered",
            "Import Keyboard Settings Action triggered",
        ]

        hydra_utils.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "basic_edit_menu_options_new_viewport.py",
            expected_lines,
            cfg_args=[level],
            run_python="--runpython",
            auto_test_mode=True,
            timeout=log_monitor_timeout,
        )
