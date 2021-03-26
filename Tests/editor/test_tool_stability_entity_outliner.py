"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.


C6308161: Tool Stability: Entity Outliner
https://testrail.agscollab.com/index.php?/cases/view/6308161
"""

import logging
import os
import pytest

# Bail on the test if ly_test_tools doesn't exist.
pytest.importorskip("ly_test_tools")

from Tests.ly_shared import hydra_lytt_test_utils as hydra_utils
import ly_test_tools.environment.file_system as file_system

logger = logging.getLogger(__name__)
test_directory = os.path.join(os.path.dirname(__file__), "EditorScripts")
log_monitor_timeout = 80


@pytest.mark.parametrize("platform", ["win_x64_vs2017"])
@pytest.mark.parametrize("configuration", ["profile"])
@pytest.mark.parametrize("project", ["SamplesProject"])
@pytest.mark.parametrize("spec", ["all"])
@pytest.mark.parametrize("level", ["tmp_level"])
class TestToolStabilityEntityOutliner(object):
    @pytest.fixture(autouse=True)
    def setup_teardown(self, request, workspace, editor, project, level):
        def teardown():
            workspace = editor.workspace
            # delete temp level
            file_system.delete([os.path.join(workspace.paths.dev(), project, "Levels", level)], True, True)

        # Setup - add the teardown finalizer
        request.addfinalizer(teardown)
        self.screenshot_path = os.path.join(editor.workspace.paths.dev(), "Cache", project, "pc", "user", "screenshots")
        # Remove any old screenshots
        self.remove_artifacts(self.screenshot_path, ".jpg")
        file_system.delete([os.path.join(workspace.paths.dev(), project, "Levels", level)], True, True)

    def remove_artifacts(self, artifact_path, suffix):
        if not os.path.isdir(artifact_path):
            return
        for file_name in os.listdir(artifact_path):
            if file_name.endswith(suffix):
                os.remove(os.path.join(artifact_path, file_name))

    @pytest.mark.BAT
    @pytest.mark.test_case_id("C6308161")
    def test_tool_stability_entity_outliner(self, request, editor, level):
        expected_lines = [
            "New entity created: True",
            "Entity Id is valid: True",
            "Entity name populated in Entity Inspector: True",
            "Entity creation undone: True",
            "Entity appeared after redo: True",
            "New layer created: True",
            "Layer name populated in Entity Inspector: True",
            "Slice created successfully: True",
            "Instantiated slice populated in Entity Inspector: True",
        ]

        unexpected_lines = [
            "New entity created: False",
            "Entity Id is valid: False",
            "Entity name populated in Entity Inspector: False",
            "Entity creation undone: False",
            "Entity appeared after redo: False",
            "New layer created: False",
            "Layer name populated in Entity Inspector: False",
            "Slice created successfully: False",
            "Instantiated slice populated in Entity Inspector: False",
        ]

        hydra_utils.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "tool_stability_entity_outliner.py",
            expected_lines,
            unexpected_lines=unexpected_lines,
            auto_test_mode=False,
            run_python="--runpython",
            cfg_args=[level],
            timeout=log_monitor_timeout,
        )
