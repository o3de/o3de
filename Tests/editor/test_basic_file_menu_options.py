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
C16780778: The File menu options function normally-New view interaction Model enabled
https://testrail.agscollab.com/index.php?/cases/view/16780778
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
class TestFileMenu(object):
    @pytest.fixture(autouse=True)
    def setup_teardown(self, request, workspace, editor, project, level):
        def teardown():
            # delete temp level
            file_system.delete([os.path.join(workspace.paths.dev(), project, "Levels", level)], True, True)

        # Setup - add the teardown finalizer
        request.addfinalizer(teardown)
        file_system.delete([os.path.join(workspace.paths.dev(), project, "Levels", level)], True, True)

    expected_lines = [
        "New Level Action triggered",
        "Open Level Action triggered",
        "Import Action triggered",
        "Save Action triggered",
        "Save As Action triggered",
        "Save Level Statistics Action triggered",
        "Switch Projects Action triggered",
        "Configure Gems Action triggered",
        "Show Log File Action triggered",
        "Resave All Slices Action triggered",
        "Upgrade Legacy Entities Action triggered",
    ]

    @pytest.mark.test_case_id("C16780778")
    def test_file_menu_after_interaction_model_toggle(self, request, editor, level):
        hydra_utils.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "basic_file_menu_options.py",
            self.expected_lines,
            cfg_args=[level],
            auto_test_mode=True,
            run_python="--runpython",
            timeout=log_monitor_timeout,
        )
