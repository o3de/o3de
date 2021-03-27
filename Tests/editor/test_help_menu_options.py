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
C16780815: Help Menu Options (New Viewport Interaction Model)
https://testrail.agscollab.com/index.php?/cases/view/16780815
"""

import os
import pytest

# Bail on the test if ly_test_tools doesn't exist.
pytest.importorskip("ly_test_tools")

from Tests.ly_shared import hydra_lytt_test_utils as hydra_utils
import ly_test_tools.environment.file_system as file_system

test_directory = os.path.join(os.path.dirname(__file__), "EditorScripts")
log_monitor_timeout = 100


@pytest.mark.parametrize('project', ['AutomatedTesting'])
@pytest.mark.parametrize('level', ['tmp_level'])
class TestHelpMenuOptions(object):
    @pytest.fixture(autouse=True)
    def setup_teardown(self, request, workspace, editor, project, level):
        def teardown():
            workspace = editor.workspace
            # delete temp level
            file_system.delete([os.path.join(workspace.paths.dev(), project, "Levels", level)], True, True)

            # kill the browser with tab titles matching 'AWS' or 'Amazon' or 'Lumberyard'
            os.popen('taskkill /FI "WINDOWTITLE eq AWS*"')
            os.popen('taskkill /FI "WINDOWTITLE eq Lumberyard*"')

        # Setup - add the teardown finalizer
        request.addfinalizer(teardown)
        file_system.delete([os.path.join(workspace.paths.dev(), project, "Levels", level)], True, True)

    expected_lines = [
        "Getting Started Action triggered",
        "Tutorials Action triggered",
        "Glossary Action triggered",
        "Lumberyard Documentation Action triggered",
        "GameLift Documentation Action triggered",
        "Release Notes Action triggered",
        "GameDev Blog Action triggered",
        "GameDev Twitch Channel Action triggered",
        "Forums Action triggered",
        "AWS Support Action triggered",
        "Give Us Feedback Action triggered",
        "About Lumberyard Action triggered",
    ]

    @pytest.mark.test_case_id("C16780815")
    def test_help_menu_after_interaction_model_toggle(self, request, editor, level):
        hydra_utils.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "help_menu_options.py",
            self.expected_lines,
            cfg_args=[level],
            timeout=log_monitor_timeout,
        )
