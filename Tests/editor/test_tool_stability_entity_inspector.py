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
C6308162: Tool Stability: Entity Inspector
https://testrail.agscollab.com/index.php?/cases/view/6308162
"""

import os
import pytest

# Bail on the test if ly_test_tools doesn't exist.
pytest.importorskip("ly_test_tools")

from Tests.ly_shared import hydra_lytt_test_utils as hydra_utils
from Tests.ly_shared import screenshot_utils as screenshot_utils
import ly_test_tools.environment.file_system as file_system

test_directory = os.path.join(os.path.dirname(__file__), "EditorScripts")
log_monitor_timeout = 80


@pytest.mark.parametrize("platform", ["win_x64_vs2017"])
@pytest.mark.parametrize("configuration", ["profile"])
@pytest.mark.parametrize("project", ["SamplesProject"])
@pytest.mark.parametrize("spec", ["all"])
@pytest.mark.parametrize("level", ["tmp_level"])
class TestToolStabilityEntityInspector(object):
    @pytest.fixture(autouse=True)
    def setup_teardown(self, request, workspace, editor, project, level):
        def teardown():
            workspace = editor.workspace
            # Save the exported screenshots and diffs in case they need to be examined later
            screenshot_utils.move_screenshots_to_artifacts(self.screenshot_path, ".jpg", workspace.artifact_manager)
            screenshot_utils.move_screenshots_to_artifacts(os.getcwd(), "_diff.jpg", workspace.artifact_manager)
            # delete temp level
            file_system.delete([os.path.join(workspace.paths.dev(), project, "Levels", level)], True, True)

        # Setup - add the teardown finalizer
        request.addfinalizer(teardown)
        self.screenshot_path = os.path.join(editor.workspace.paths.dev(), "Cache", project, "pc", "user", "screenshots")
        # Remove any old screenshots and diff comparisons
        self.remove_artifacts(self.screenshot_path, ".jpg")
        self.remove_artifacts(os.getcwd(), "_diff.jpg")
        file_system.delete([os.path.join(workspace.paths.dev(), project, "Levels", level)], True, True)

    def remove_artifacts(self, artifact_path, suffix):
        if not os.path.isdir(artifact_path):
            return
        for file_name in os.listdir(artifact_path):
            if file_name.endswith(suffix):
                os.remove(os.path.join(artifact_path, file_name))

    @pytest.mark.test_case_id("C6308162")
    def test_tool_stability_entity_inspector(self, request, editor, level):
        golden_screenshot_path = os.path.join(editor.workspace.paths.dev(), "Tests", "editor", "goldenimages")
        expected_lines = [
            "Entity has a Mesh component",
            "Screenshot Taken",
            "focus changed",
            "Entity with Mesh component is filtered by Entity outliner",
        ]

        hydra_utils.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "tool_stability_entity_inspector.py",
            expected_lines,
            cfg_args=[level],
            auto_test_mode=False,
            run_python="--runpython",
            timeout=log_monitor_timeout,
        )

        screenshot_utils.compare_golden_image(
            0.95,
            "screenshot0000.jpg",
            self.screenshot_path,
            "C6308162_tool_stability_entity_inspector.jpg",
            golden_screenshot_path,
        )
