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
C16929882: Required Components
https://testrail.agscollab.com/index.php?/cases/view/16929882
"""

import os
import pytest

# Bail on the test if ly_test_tools doesn't exist.
pytest.importorskip("ly_test_tools")

from Tests.hydra.hydra_utils import launch_test_case
from Tests.ly_shared import hydra_lytt_test_utils as hydra_utils
import ly_test_tools.environment.file_system as file_system

test_directory = os.path.join(os.path.dirname(__file__), "EditorScripts")
log_monitor_timeout = 60


@pytest.mark.parametrize("platform", ["win_x64_vs2017"])
@pytest.mark.parametrize("configuration", ["profile"])
@pytest.mark.parametrize("project", ["SamplesProject"])
@pytest.mark.parametrize("spec", ["all"])
@pytest.mark.parametrize("level", ["tmp_level"])
class TestRequiredComponents(object):
    @pytest.fixture(autouse=True)
    def setup_teardown(self, request, workspace, editor, project, level):
        def teardown():
            workspace = editor.workspace
            # delete temp level
            file_system.delete([os.path.join(workspace.paths.dev(), project, "Levels", level)], True, True)

        # Setup - add the teardown finalizer
        request.addfinalizer(teardown)
        file_system.delete([os.path.join(workspace.paths.dev(), project, "Levels", level)], True, True)

    @pytest.mark.test_case_id("C16929882")
    def test_required_components(self, request, editor, level):
        expected_lines = [
            "Required Component test successful."
        ]
        unexpected_lines = [
            "New level failed.",
            "Failed to create entity.",
            "Failed to find editor window.",
            "Failed to find Entity Inspector.",
            "Failed to find Tube Shape id.",
            "Failed to find Spline id.",
            "Failed to add Tube Shape to entity.",
            "Tube is enabled when first created.",
            "Failed to find component list.",
            "Failed to find tube shape inspector.",
            "Unable to find tube header frame",
            "Unable to find tube container frame",
            "Unable to find add required component button",
            "Spline component already exists.",
            "Failed to add Spline component to entity.",
            "Tube is not enabled.",
            "Missing component warning still exists.",
            "Test Failed."
        ]

        hydra_utils.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "required_components.py",
            expected_lines,
            unexpected_lines=unexpected_lines,
            auto_test_mode=False,
            run_python="--runpython",
            cfg_args=[level],
            timeout=log_monitor_timeout,
        )