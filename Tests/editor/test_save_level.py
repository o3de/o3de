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
C15167490: Save a level
https://testrail.agscollab.com/index.php?/cases/view/15167490
"""

import logging
import os
import pytest

# Bail on the test if ly_test_tools doesn't exist.
pytest.importorskip("ly_test_tools")
import ly_test_tools.log.log_monitor
import ly_test_tools.environment.waiter as waiter

from Tests.ly_shared import hydra_lytt_test_utils as hydra_utils
import ly_test_tools.environment.file_system as file_system

logger = logging.getLogger(__name__)
test_directory = os.path.join(os.path.dirname(__file__), "EditorScripts")
log_monitor_timeout = 60


@pytest.mark.parametrize("platform", ["win_x64_vs2017"])
@pytest.mark.parametrize("configuration", ["profile"])
@pytest.mark.parametrize("project", ["SamplesProject"])
@pytest.mark.parametrize("spec", ["all"])
@pytest.mark.parametrize("level", ["tmp_level"])
class TestSaveALevel(object):
    @pytest.fixture(autouse=True)
    def setup_teardown(self, request, workspace, editor, project, level):
        def teardown():
            workspace = editor.workspace
            # delete temp level
            file_system.delete([os.path.join(workspace.paths.dev(), project, "Levels", level)], True, True)
            file_system.delete([os.path.join(workspace.paths.dev(), project, "Levels", "tmp_new_level")], True, True)

        # Setup - add the teardown finalizer
        request.addfinalizer(teardown)
        file_system.delete([os.path.join(workspace.paths.dev(), project, "Levels", level)], True, True)
        file_system.delete([os.path.join(workspace.paths.dev(), project, "Levels", "tmp_new_level")], True, True)

    @pytest.mark.test_case_id("C15167490")
    @pytest.mark.BAT
    def test_save_level(self, request, editor, level):
    
        expected_lines = [
            "Entity created",
            "The 'Save Level As' dialog appeared",
            "Save As Action triggered",
            "A new level file is created and saved with the name provided.: SUCCESS",
            "Made a change in level",
            "Save Action triggered"
        ]

        hydra_utils.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "save_level.py",
            expected_lines,
            auto_test_mode=False,
            run_python="--runpython",
            cfg_args=[level],
            timeout=log_monitor_timeout,
        )