"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

C6376081: Basic Function: Docked/Undocked Tools
"""

import os
import pytest

# Bail on the test if ly_test_tools doesn't exist.
pytest.importorskip('ly_test_tools')
import ly_test_tools.environment.file_system as file_system
import editor_python_test_tools.hydra_test_utils as hydra

test_directory = os.path.join(os.path.dirname(__file__), "EditorScripts")
log_monitor_timeout = 180


@pytest.mark.parametrize('project', ['AutomatedTesting'])
@pytest.mark.parametrize('level', ['tmp_level'])
@pytest.mark.usefixtures("automatic_process_killer")
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
class TestDocking(object):

    @pytest.fixture(autouse=True)
    def setup_teardown(self, request, workspace, project, level):
        def teardown():
            file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)

        request.addfinalizer(teardown)

        file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)

    @pytest.mark.test_case_id("C6376081")
    @pytest.mark.SUITE_sandbox
    def test_Docking_BasicDockedTools(self, request, editor, level, launcher_platform):
        expected_lines = [
            "The tools are all docked together in a tabbed widget",
            "Entity Outliner works when docked, can select an Entity",
            "Entity Inspector works when docked, Entity name changed to DifferentName",
            "Hello, world!"    # This line verifies the Console is working while docked
        ]

        hydra.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "Docking_BasicDockedTools.py",
            expected_lines,
            cfg_args=[level],
            timeout=log_monitor_timeout,
        )
