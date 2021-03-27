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
C15167491: Export a level
https://testrail.agscollab.com/index.php?/cases/view/15167491
"""

import os
import pytest

# Bail on the test if ly_test_tools doesn't exist.
pytest.importorskip("ly_test_tools")

from ..ly_shared import hydra_lytt_test_utils as hydra_utils
import ly_test_tools.environment.file_system as file_system

test_directory = os.path.join(os.path.dirname(__file__), "EditorScripts")
log_monitor_timeout = 60


@pytest.mark.parametrize("platform", ["win_x64_vs2017"])
@pytest.mark.parametrize("configuration", ["profile"])
@pytest.mark.parametrize("project", ["SamplesProject"])
@pytest.mark.parametrize("spec", ["all"])
@pytest.mark.parametrize("level", ["tmp_level"])
class TestExportLevel(object):
    @pytest.fixture(autouse=True)
    def setup_teardown(self, request, legacy_workspace, project, level):
        def teardown():
            # delete temp level
            file_system.delete([os.path.join(legacy_workspace.paths.dev(), project, "Levels", level)], True, True)

        # Setup - add the teardown finalizer
        request.addfinalizer(teardown)
        file_system.delete([os.path.join(workspace.paths.dev(), project, "Levels", level)], True, True)

    @pytest.mark.test_case_id("C15167491")
    def test_export_level(self, request, legacy_editor, level):
        expected_lines = [
            "Game->Export to Engine Action triggered",
            "level.pak file exists",
            "Export to the game was successfully done.",
        ]
        hydra_utils.launch_and_validate_results(
            request,
            test_directory,
            legacy_editor,
            "export_level.py",
            expected_lines,
            cfg_args=[level],
            timeout=log_monitor_timeout,
        )
