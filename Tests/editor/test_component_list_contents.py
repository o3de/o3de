"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

C16798661: Component List Contents
https://testrail.agscollab.com/index.php?/cases/view/16798661
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
class TestAddDeleteComponents(object):
    @pytest.fixture(autouse=True)
    def setup_teardown(self, request, workspace, editor, project, level):
        def teardown():
            workspace = editor.workspace
            # delete temp level
            file_system.delete([os.path.join(workspace.paths.dev(), project, "Levels", level)], True, True)

        # Setup - add the teardown finalizer
        request.addfinalizer(teardown)
        file_system.delete([os.path.join(workspace.paths.dev(), project, "Levels", level)], True, True)

    @pytest.mark.test_case_id("C16798661")
    def test_component_list_contents(self, request, editor, level):
        expected_lines = [
            "Entity Created",
            "Successfully collapsed category.",
            "Successfully expanded category.",
            "Box Shape successfully added.",
        ]

        unexpected_lines = ["Box Shape already existed on Entity."]

        hydra_utils.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "component_list_contents.py",
            expected_lines,
            unexpected_lines=unexpected_lines,
            cfg_args=[level],
            auto_test_mode=False,
            run_python="--runpython",
            timeout=log_monitor_timeout,
        )
