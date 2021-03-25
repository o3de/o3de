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
C1508364 : Select over 1,000+ entities that are open in the Entity Outliner at one time
https://testrail.agscollab.com/index.php?/cases/view/1508364
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
class TestSelectMultipleEntitiesEntityOutliner(object):
    @pytest.fixture(autouse=True)
    def setup_teardown(self, request, workspace, editor, project, level):
        def teardown():
            workspace = editor.workspace
            # delete temp level
            file_system.delete([os.path.join(workspace.paths.dev(), project, "Levels", level)], True, True)

        # Setup - add the teardown finalizer
        request.addfinalizer(teardown)
        file_system.delete([os.path.join(workspace.paths.dev(), project, "Levels", level)], True, True)

    @pytest.mark.test_case_id("C1508364")
    def test_select_multiple_entities_entityoutliner(self, request, editor, level):
        expected_lines = [
            "1000 entities created",
            "The Entity Inspector shows an accurate count of selected entities (1 element): True",
            "CTRL+click worked for adding selected elements (1 element): True",
            "The Entity Inspector shows an accurate count of selected entities (2 elements): True",
            "CTRL+click worked for adding selected elements (2 elements): True",
            "The Entity Inspector shows an accurate count of selected entities (3 elements): True",
            "CTRL+click worked for adding selected elements (3 elements): True",
            "Selected entities are highlighted in the Entity Outliner",
            "The Entity Inspector shows an accurate count of selected entities (1000 elements): True",
        ]

        unexpected_lines = [
            "The Entity Inspector shows an accurate count of selected entities (1 element): False",
            "CTRL+click worked for adding selected elements (1 element): False",
            "The Entity Inspector shows an accurate count of selected entities (2 elements): False",
            "CTRL+click worked for adding selected elements (2 elements): False",
            "The Entity Inspector shows an accurate count of selected entities (3 elements): False",
            "CTRL+click worked for adding selected elements (3 elements): False",
            "The Entity Inspector shows an accurate count of selected entities (1000 elements): False",
        ]

        hydra_utils.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "select_multiple_entities_entityoutliner.py",
            expected_lines,
            unexpected_lines=unexpected_lines,
            cfg_args=[level],
            auto_test_mode=False,
            run_python="--runpython",
            timeout=log_monitor_timeout,
        )
