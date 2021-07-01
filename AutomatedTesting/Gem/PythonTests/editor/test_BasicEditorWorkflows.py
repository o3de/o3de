"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
import pytest

# Bail on the test if ly_test_tools doesn't exist.
pytest.importorskip('ly_test_tools')
import ly_test_tools.environment.file_system as file_system
import ly_test_tools._internal.pytest_plugin as internal_plugin
import editor_python_test_tools.hydra_test_utils as hydra

test_directory = os.path.join(os.path.dirname(__file__), "EditorScripts")
log_monitor_timeout = 180


@pytest.mark.parametrize('project', ['AutomatedTesting'])
@pytest.mark.parametrize('level', ['tmp_level'])
@pytest.mark.usefixtures("automatic_process_killer")
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
class TestBasicEditorWorkflows(object):

    @pytest.fixture(autouse=True)
    def setup_teardown(self, request, workspace, project, level):
        def teardown():
            file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)

        request.addfinalizer(teardown)

        file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)

    @pytest.mark.test_case_id("C6351273", "C6384955", "C16929880", "C15167490", "C15167491")
    @pytest.mark.SUITE_main
    def test_BasicEditorWorkflows_LevelEntityComponentCRUD(self, request, editor, level, launcher_platform):

        # Skip test if running against Debug build
        if "debug" in internal_plugin.build_directory:
            pytest.skip("Does not execute against debug builds.")

        expected_lines = [
            "Create and load new level: True",
            "New entity creation: True",
            "Create entity hierarchy: True",
            "Add component: True",
            "Component update: True",
            "Remove component: True",
            "Save and Export: True",
            "BasicEditorWorkflows_LevelEntityComponent:  result=SUCCESS",
        ]

        hydra.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "BasicEditorWorkflows_LevelEntityComponentCRUD.py",
            expected_lines,
            cfg_args=[level],
            timeout=log_monitor_timeout,
            auto_test_mode=False
        )

    @pytest.mark.test_case_id("C6351273", "C6384955", "C16929880", "C15167490", "C15167491")
    @pytest.mark.SUITE_main
    @pytest.mark.REQUIRES_gpu
    def test_BasicEditorWorkflows_GPU_LevelEntityComponentCRUD(self, request, editor, level, launcher_platform):

        # Skip test if running against Debug build
        if "debug" in internal_plugin.build_directory:
            pytest.skip("Does not execute against debug builds.")

        expected_lines = [
            "Create and load new level: True",
            "New entity creation: True",
            "Create entity hierarchy: True",
            "Add component: True",
            "Component update: True",
            "Remove component: True",
            "Save and Export: True",
            "BasicEditorWorkflows_LevelEntityComponent:  result=SUCCESS",
        ]

        hydra.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "BasicEditorWorkflows_LevelEntityComponentCRUD.py",
            expected_lines,
            cfg_args=[level],
            timeout=log_monitor_timeout,
            auto_test_mode=False,
            null_renderer=False
        )
