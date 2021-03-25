"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

C6376081: Basic Function: Docked/Undocked Tools
https://testrail.agscollab.com/index.php?/cases/view/6376081
"""

import os
import pytest

# Bail on the test if ly_test_tools doesn't exist.
pytest.importorskip("ly_test_tools")

from Tests.ly_shared import hydra_lytt_test_utils as hydra_utils
import ly_test_tools.environment.file_system as file_system

test_directory = os.path.join(os.path.dirname(__file__), "EditorScripts")
log_monitor_timeout = 60


@pytest.mark.parametrize("platform", ["win_x64_vs2017"])
@pytest.mark.parametrize("configuration", ["profile"])
@pytest.mark.parametrize("project", ["SamplesProject"])
@pytest.mark.parametrize("spec", ["all"])
@pytest.mark.parametrize("level", ["tmp_level"])
class TestBasicDockedTools(object):
    @pytest.fixture(autouse=True)
    def setup_teardown(self, request, workspace, editor, project, level):
        def teardown():
            workspace = editor.workspace
            # delete temp level
            file_system.delete([os.path.join(workspace.paths.dev(), project, "Levels", level)], True, True)

        # Setup - add the teardown finalizer
        request.addfinalizer(teardown)
        file_system.delete([os.path.join(workspace.paths.dev(), project, "Levels", level)], True, True)

    @pytest.mark.test_case_id("C6376081")
    def test_basic_docked_tools(self, request, editor, level):
        expected_lines = [
            "The tools are all docked together in a tabbed widget",
            "Entity Outliner works when docked, can select an Entity",
            "Entity Inspector works when docked, Entity name changed to DifferentName",
            "e_Vegetation=1" # This line verifies the Console is working while docked, since it will print out this command
        ]

        hydra_utils.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "basic_docked_tools.py",
            expected_lines,
            cfg_args=[level],
            timeout=log_monitor_timeout,
        )
