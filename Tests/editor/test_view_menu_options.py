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
C16780807: The View menu options function normally - New view interaction Model enabled
https://testrail.agscollab.com/index.php?/cases/view/16780807
"""

import os
import pytest

# Bail on the test if ly_test_tools doesn't exist.
pytest.importorskip("ly_test_tools")

from ..ly_shared import hydra_lytt_test_utils as hydra_utils
import ly_test_tools.environment.file_system as file_system

test_directory = os.path.join(os.path.dirname(__file__), "EditorScripts")
log_monitor_timeout = 100


@pytest.mark.parametrize('project', ['AutomatedTesting'])
@pytest.mark.parametrize('level', ['tmp_level'])
class TestViewMenu(object):
    @pytest.fixture(autouse=True)
    def setup_teardown(self, request, workspace, project, level):
        def teardown():
            # delete temp level
            file_system.delete([os.path.join(workspace.paths.dev(), project, "Levels", level)], True, True)

        # Setup - add the teardown finalizer
        request.addfinalizer(teardown)
        file_system.delete([os.path.join(workspace.paths.dev(), project, "Levels", level)], True, True)

    expected_lines = [
        "Center on Selection Action triggered",
        "Show Quick Access Bar Action triggered",
        "Default Layout Action triggered",
        "User Legacy Layout Action triggered",
        "Save Layout Action triggered",
        "Restore Default Layout Action triggered",
        "Wireframe Action triggered",
        "Grid Settings Action triggered",
        "Configure Layout Action triggered",
        "Goto Coordinates Action triggered",
        "Center on Selection Action triggered",
        "Goto Location Action triggered",
        "Remember Location Action triggered",
        "Change Move Speed Action triggered",
        "Switch Camera Action triggered",
        "Show/Hide Helpers Action triggered",
        "Refresh Style Action triggered",
    ]

    @pytest.mark.test_case_id("C16780807")
    def test_view_menu_options_interaction_model(self, request, editor, level):
        hydra_utils.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "view_menu_after_interaction_model_toggle.py",
            self.expected_lines,
            cfg_args=[level],
            auto_test_mode=False,
            run_python="--runpython",
            timeout=log_monitor_timeout,
        )
