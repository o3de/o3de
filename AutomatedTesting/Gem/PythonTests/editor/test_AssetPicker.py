"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

"""
C13751579: Asset Picker UI/UX
"""

import os
import pytest

# Bail on the test if ly_test_tools doesn't exist.
pytest.importorskip('ly_test_tools')
import ly_test_tools.environment.file_system as file_system
import editor_python_test_tools.hydra_test_utils as hydra

test_directory = os.path.join(os.path.dirname(__file__), "EditorScripts")
log_monitor_timeout = 90


@pytest.mark.parametrize('project', ['AutomatedTesting'])
@pytest.mark.parametrize('level', ['tmp_level'])
@pytest.mark.usefixtures("automatic_process_killer")
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
class TestAssetPicker(object):

    @pytest.fixture(autouse=True)
    def setup_teardown(self, request, workspace, project, level):
        def teardown():
            file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)

        request.addfinalizer(teardown)

        file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)

    @pytest.mark.test_case_id("C13751579", "C1508814")
    @pytest.mark.SUITE_periodic
    @pytest.mark.xfail      # ATOM-15493
    def test_AssetPicker_UI_UX(self, request, editor, level, launcher_platform):
        expected_lines = [
            "TestEntity Entity successfully created",
            "Mesh component was added to entity",
            "Entity has a Mesh component",
            "Mesh Asset: Asset Picker title for Mesh: Pick ModelAsset",
            "Mesh Asset: Scroll Bar is not visible before expanding the tree: True",
            "Mesh Asset: Top level folder initially collapsed: True",
            "Mesh Asset: Top level folder expanded: True",
            "Mesh Asset: Nested folder initially collapsed: True",
            "Mesh Asset: Nested folder expanded: True",
            "Mesh Asset: Scroll Bar appeared after expanding tree: True",
            "Mesh Asset: Nested folder collapsed: True",
            "Mesh Asset: Top level folder collapsed: True",
            "Mesh Asset: Expected Assets populated in the file picker: True",
            "Widget Move Test: True",
            "Widget Resize Test: True",
            "Asset assigned for ok option: True",
            "Asset assigned for enter option: True",
            "AssetPicker_UI_UX:  result=SUCCESS"
        ]

        hydra.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "AssetPicker_UI_UX.py",
            expected_lines,
            cfg_args=[level],
            run_python="--runpython",
            auto_test_mode=False,
            timeout=log_monitor_timeout,
        )
