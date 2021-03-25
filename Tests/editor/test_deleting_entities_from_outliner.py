"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

C1510643: Deleting Entities in the Entity Outliner
https://testrail.agscollab.com/index.php?/cases/view/1510643
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
class TestDeletingEntitiesFromOutliner(object):
    @pytest.fixture(autouse=True)
    def setup_teardown(self, request, workspace, editor, project, level):
        def teardown():
            workspace = editor.workspace
            # delete temp level
            file_system.delete([os.path.join(workspace.paths.dev(), project, "Levels", level)], True, True)

        # Setup - add the teardown finalizer
        request.addfinalizer(teardown)
        file_system.delete([os.path.join(workspace.paths.dev(), project, "Levels", level)], True, True)

    @pytest.mark.test_case_id("C1510643")
    def test_deleting_entities_from_outliner(self, request, editor, level):
        expected_lines = [
            "Parent Entity created",
            "Child Entities created",
            "Child entity 1 deleted succesfully",
            "Deleting a Child entity does not delete the parent",
            "Child entity restored successfully",
            "Parent entity deleted succesfully",
            "Deleting the Parent entity deletes all child entities linked to it",
            "Entity hierarchy is restored",
        ]

        hydra_utils.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "deleting_entities_from_outliner.py",
            expected_lines,
            cfg_args=[level],
            timeout=log_monitor_timeout,
        )
