"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
import pytest

# Bail on the test if ly_test_tools doesn't exist.
pytest.importorskip('ly_test_tools')
import ly_test_tools.environment.file_system as file_system
import ly_test_tools.environment.process_utils as process_utils
import editor_python_test_tools.hydra_test_utils as hydra

test_directory = os.path.join(os.path.dirname(__file__), "EditorScripts")
log_monitor_timeout = 180


@pytest.mark.parametrize('project', ['AutomatedTesting'])
@pytest.mark.parametrize('level', ['tmp_level'])
@pytest.mark.usefixtures("automatic_process_killer")
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
class TestMenus(object):

    @pytest.fixture(autouse=True)
    def setup_teardown(self, request, workspace, project, level):
        def teardown():
            file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)
            process_utils.kill_processes_named("o3de", ignore_extensions=True)  # Kill ProjectManager windows

        request.addfinalizer(teardown)

        file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)

    @pytest.mark.test_case_id("C16780783", "C2174438")
    @pytest.mark.SUITE_sandbox
    def test_Menus_EditMenuOptions_Work(self, request, editor, level, launcher_platform):
        expected_lines = [
            "Undo Action triggered",
            "Redo Action triggered",
            "Duplicate Action triggered",
            "Delete Action triggered",
            "Select All Action triggered",
            "Invert Selection Action triggered",
            "Toggle Pivot Location Action triggered",
            "Reset Entity Transform",
            "Reset Manipulator",
            "Reset Transform (Local) Action triggered",
            "Reset Transform (World) Action triggered",
            "Hide Selection Action triggered",
            "Show All Action triggered",
            "Snap angle Action triggered",
            "Move Action triggered",
            "Rotate Action triggered",
            "Scale Action triggered",
            "Global Preferences Action triggered",
            "Graphics Settings Action triggered",
            "Editor Settings Manager Action triggered",
            "Very High Action triggered",
            "High Action triggered",
            "Medium Action triggered",
            "Low Action triggered",
            "Customize Keyboard Action triggered",
            "Export Keyboard Settings Action triggered",
            "Import Keyboard Settings Action triggered",
            "Menus_EditMenuOptions:  result=SUCCESS"
        ]

        hydra.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "Menus_EditMenuOptions.py",
            expected_lines,
            cfg_args=[level],
            run_python="--runpython",
            timeout=log_monitor_timeout
        )

    @pytest.mark.test_case_id("C16780807")
    @pytest.mark.SUITE_periodic
    def test_Menus_ViewMenuOptions_Work(self, request, editor, level, launcher_platform):
        expected_lines = [
            "Center on Selection Action triggered",
            "Show Quick Access Bar Action triggered",
            "Configure Layout Action triggered",
            "Go to Position Action triggered",
            "Center on Selection Action triggered",
            "Go to Location Action triggered",
            "Remember Location Action triggered",
            "Change Move Speed Action triggered",
            "Switch Camera Action triggered",
            "Show/Hide Helpers Action triggered",
            "Refresh Style Action triggered",
        ]
        hydra.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "Menus_ViewMenuOptions.py",
            expected_lines,
            cfg_args=[level],
            run_python="--runpython",
            timeout=log_monitor_timeout
        )

    @pytest.mark.test_case_id("C16780778")
    @pytest.mark.SUITE_sandbox
    @pytest.mark.xfail      # LYN-4208
    def test_Menus_FileMenuOptions_Work(self, request, editor, level, launcher_platform):
        expected_lines = [
            "New Level Action triggered",
            "Open Level Action triggered",
            "Import Action triggered",
            "Save Action triggered",
            "Save As Action triggered",
            "Save Level Statistics Action triggered",
            "Edit Project Settings Action triggered",
            "Edit Platform Settings Action triggered",
            "New Project Action triggered",
            "Open Project Action triggered",
            "Show Log File Action triggered",
            "Resave All Slices Action triggered",
            "Exit Action triggered",
        ]

        hydra.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "Menus_FileMenuOptions.py",
            expected_lines,
            cfg_args=[level],
            run_python="--runpython",
            timeout=log_monitor_timeout
        )
