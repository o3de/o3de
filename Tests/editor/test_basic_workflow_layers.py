"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

C6384949: Basic Workflow: Layers
https://testrail.agscollab.com/index.php?/cases/view/6384949
"""
import os
import pytest

# Bail on the test if ly_test_tools doesn't exist.
pytest.importorskip("ly_test_tools")

from Tests.ly_shared import hydra_lytt_test_utils as hydra_utils
import ly_test_tools.environment.file_system as file_system

test_directory = os.path.join(os.path.dirname(__file__), "EditorScripts")
log_monitor_timeout = 100


@pytest.mark.parametrize("platform", ["win_x64_vs2017"])
@pytest.mark.parametrize("configuration", ["profile"])
@pytest.mark.parametrize("project", ["SamplesProject"])
@pytest.mark.parametrize("spec", ["all"])
@pytest.mark.parametrize("level", ["tmp_level"])
class TestBasicWorkflowLayers(object):
    @pytest.fixture(autouse=True)
    def setup_teardown(self, request, workspace, editor, project, level):
        def teardown():
            workspace = editor.workspace

            # delete temp level
            file_system.delete([os.path.join(workspace.paths.dev(), project, "Levels", level)], True, True)

        # Setup - add the teardown finalizer
        request.addfinalizer(teardown)
        file_system.delete([os.path.join(workspace.paths.dev(), project, "Levels", level)], True, True)

    @pytest.mark.test_case_id("C6384949")
    @pytest.mark.BAT
    def test_basic_workflow_layers(self, request, editor, level):
        expected_lines = [
            "First Layer is created",
            "First Entity is created",
            "Original Check: The parent entity of Entity1 is : Layer1",
            "Second Entity is created",
            "Original Check: The parent entity of Entity2 is : Layer1",
            "After Drag: The parent entity of Entity1 is :",
            "After Drag: The parent entity of Entity2 is :",
            "Second Layer is created",
            "After Drag: The parent entity of Layer2 is : Layer1",
            "ChildPolished event received",
        ]

        hydra_utils.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "basic_workflow_layers.py",
            expected_lines,
            auto_test_mode=False,
            run_python="--runpython",
            cfg_args=[level],
            timeout=log_monitor_timeout,
        )
