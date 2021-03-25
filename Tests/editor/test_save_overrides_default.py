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
C2654251: Adding and deleting entities to a slice should be included on save overrides by default
https://testrail.agscollab.com/index.php?/cases/view/2654251
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
class TestSaveOverridesDefault(object):
    @pytest.fixture(autouse=True)
    def setup_teardown(self, request, workspace, editor, project, level):
        def teardown():
            workspace = editor.workspace
            # delete temp level
            file_system.delete([os.path.join(workspace.paths.dev(), project, "Levels", level)], True, True)
            file_system.delete([os.path.join(workspace.paths.dev(), project, "test_slice.slice")], True, True)

        # Setup - add the teardown finalizer
        request.addfinalizer(teardown)
        file_system.delete([os.path.join(workspace.paths.dev(), project, "Levels", level)], True, True)
        file_system.delete([os.path.join(workspace.paths.dev(), project, "test_slice.slice")], True, True)

    @pytest.mark.test_case_id("C2654251")
    def test_save_overrides_default(self, request, editor, level):
        expected_lines = [
            "Slice Created: True",
            "Child 1 deleted change checked by default: True",
            "Child 2 added change checked by default: True",
        ]

        hydra_utils.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "save_overrides_default.py",
            expected_lines,
            cfg_args=[level],
            run_python="--runpython",
            auto_test_mode=False,
            timeout=log_monitor_timeout,
        )
