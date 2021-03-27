"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.


C2202976: Entity Selection - Entity Outliner
https://testrail.agscollab.com/index.php?/cases/view/2202976
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
log_monitor_timeout = 60


@pytest.mark.parametrize('project', ['AutomatedTesting'])
@pytest.mark.parametrize('level', ['tmp_level'])
class TestEntitySelectionEntityOutliner(object):
    @pytest.fixture(autouse=True)
    def setup_teardown(self, request, workspace, editor, project, level):
        def teardown():
            workspace = editor.workspace
            # delete temp level
            file_system.delete([os.path.join(workspace.paths.dev(), project, "Levels", level)], True, True)

        # Setup - add the teardown finalizer
        request.addfinalizer(teardown)
        file_system.delete([os.path.join(workspace.paths.dev(), project, "Levels", level)], True, True)

        
    @pytest.mark.test_case_id("C2202976")
    def test_entity_selection_entity_outliner(self, request, editor, level):
        expected_lines = [
            "Entity2 Entity successfully created",
            "Entity3 Entity successfully created",
            "Entity4 Entity successfully created",
            "Single entity selected on first click: True",
            "Single entity selected on second click: True",
            "CTRL+click worked for adding selected elements (2 elements): True",
            "CTRL+click worked for adding selected elements (3 elements): True",
            "CTRL+click worked for removing already selected elements: True"
        ]
        unexpected_lines = [
            "Single entity selected on first click: False",
            "Single entity selected on second click: False",
            "CTRL+click worked for adding selected elements (2 elements): False",
            "CTRL+click worked for adding selected elements (3 elements): False",
            "CTRL+click worked for removing already selected elements: False"
        ]
        hydra_utils.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "entity_selection_entity_outliner.py",
            expected_lines,
            unexpected_lines=unexpected_lines,
            run_python="--runpython",
            cfg_args=[level],
            timeout=log_monitor_timeout,
        )

