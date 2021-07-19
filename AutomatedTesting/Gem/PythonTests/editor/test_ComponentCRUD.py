"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

"""
C16929880: Add Delete Components
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
class TestComponentCRUD(object):

    @pytest.fixture(autouse=True)
    def setup_teardown(self, request, workspace, project, level):
        def teardown():
            file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)

        request.addfinalizer(teardown)

        file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)

    @pytest.mark.test_case_id("C16929880", "C16877220")
    @pytest.mark.SUITE_periodic
    @pytest.mark.BAT
    def test_ComponentCRUD_Add_Delete_Components(self, request, editor, level, launcher_platform):
        expected_lines = [
            "Entity Created",
            "Box Shape found",
            "Box Shape Component added: True",
            "Mesh found",
            "Mesh Component added: True",
            "Mesh Component deleted: True",
            "Mesh Component deletion undone: True",
        ]

        hydra.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "ComponentCRUD_Add_Delete_Components.py",
            expected_lines,
            cfg_args=[level],
            auto_test_mode=False,
            timeout=log_monitor_timeout
        )
