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
C17218881: Basic Function: Locking & Hiding
https://testrail.agscollab.com/index.php?/cases/view/17218881
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
class TestBasicFunctionLockHide(object):
    @pytest.fixture(autouse=True)
    def setup_teardown(self, request, workspace, editor, project, level):
        def teardown():
            workspace = editor.workspace
            # delete temp level
            file_system.delete([os.path.join(workspace.paths.dev(), project, "Levels", level)], True, True)

        # Setup - add the teardown finalizer
        request.addfinalizer(teardown)
        file_system.delete([os.path.join(workspace.paths.dev(), project, "Levels", level)], True, True)

    expected_lines = [
        "Entity2 Entity successfully created",
        "Entity3 Entity successfully created",
        "Entity4 Entity successfully created",
        "Entity5 Entity successfully created",
        "Child entities are hidden when parent entity is hidden: True",
        "Child entities are shown when parent entity is shown: True",
        "Child entities are locked when parent entity is locked: True",
        "Child entities are unlocked when parent entity is unlocked: True",
        "Entity6 Entity successfully created",
        "Entity7 Entity successfully created",
        "The parent entity of Entity6 is Entity2",
        "The parent entity of Entity7 is Entity2",
    ]

    @pytest.mark.test_case_id("C17218881")
    def test_basic_function_lock_hide(self, request, editor, level):
        hydra_utils.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "basic_function_lock_hide.py",
            self.expected_lines,
            auto_test_mode=False,
            cfg_args=[level],
            timeout=log_monitor_timeout,
        )
