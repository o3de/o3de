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
C1506899: Unsaved Changes Pop-Up
https://testrail.agscollab.com/index.php?/cases/view/1506899
"""

import os
import pytest

# Bail on the test if ly_test_tools doesn't exist.
pytest.importorskip("ly_test_tools")

from Tests.ly_shared import hydra_lytt_test_utils as hydra_utils
import ly_test_tools.environment.file_system as file_system

test_directory = os.path.join(os.path.dirname(__file__), "EditorScripts")


@pytest.mark.parametrize("platform", ["win_x64_vs2017"])
@pytest.mark.parametrize("configuration", ["profile"])
@pytest.mark.parametrize("project", ["SamplesProject"])
@pytest.mark.parametrize("spec", ["all"])
@pytest.mark.parametrize("level", ["tmp_level"])
class TestUnsavedChangesPopup(object):
    @pytest.fixture(autouse=True)
    def setup_teardown(self, request, workspace, editor, project, level):
        def teardown():
            workspace = editor.workspace
            # delete temp level
            file_system.delete([os.path.join(workspace.paths.dev(), project, "Levels", level)], True, True)

        # Setup - add the teardown finalizer
        request.addfinalizer(teardown)
        file_system.delete([os.path.join(workspace.paths.dev(), project, "Levels", level)], True, True)

    @pytest.mark.test_case_id("C1506899")
    def test_unsaved_changes_popup(self, request, editor, level):
        expected_lines = [
            "Message Box opened",
            "'CANCEL' button working as expected: True",
            "Asset Editor is visible after clicking YES: True",
            "'YES' button working as expected",
            "Asset Editor is not visible after clicking NO: True",
            "'NO' button working as expected",
            "Asset Editor closed: True",
        ]

        unexpected_lines = [
            "'CANCEL' button working as expected: False",
            "Asset Editor is visible after clicking YES: False",
            "Asset Editor is not visible after clicking NO: False",
            "Asset Editor closed: False",
        ]
        hydra_utils.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "unsaved_changes_popup.py",
            expected_lines,
            unexpected_lines=unexpected_lines,
            cfg_args=[level],
            auto_test_mode=False # Need this so that the modal dialogs don't get dismissed automatically
        )
