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
C6321557: Basic Function: Graphics Settings
https://testrail.agscollab.com/index.php?/cases/view/6321557
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
class TestGraphicsSettings(object):
    @pytest.fixture(autouse=True)
    def setup_teardown(self, request, workspace, editor, project, level):
        def teardown():
            workspace = editor.workspace
            # delete graphics settings file
            file_system.delete(
                [os.path.join(workspace.paths.dev(), "SamplesProject", "Config", "Spec", "pc_low.cfg")],
                True,
                True
            )

        # Setup - add the teardown finalizer
        request.addfinalizer(teardown)

    @pytest.mark.test_case_id("C6321557")
    def test_conflicting_components(self, request, editor, level):
        expected_lines = [
            "Graphics Settings test successful."
        ]
        unexpected_lines = [
            "Failed to find Graphics Settings action.",
            "Failed to find graphics settings dialog.",
            "Failed to find Save button",
            "Save button is enabled before change.",
            "Failed to find spinbox.",
            "Failed to change value.",
            "Save button is not enabled after change.",
            "Failed to find warning dialog.",
            "Unable to find message box in warning dialog",
            "Unexpected text in warning dialog.",
            "Failed to find yes button.",
            "Test Failed."
        ]

        hydra_utils.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "graphics_settings.py",
            expected_lines,
            unexpected_lines=unexpected_lines,
            auto_test_mode=False,
            run_python="--runpython",
            cfg_args=[level],
            timeout=log_monitor_timeout,
        )

