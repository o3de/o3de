"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
import pytest
import logging

# Bail on the test if ly_test_tools doesn't exist.
pytest.importorskip("ly_test_tools")
import ly_test_tools.environment.file_system as file_system
import editor_python_test_tools.hydra_test_utils as hydra

logger = logging.getLogger(__name__)
test_directory = os.path.join(os.path.dirname(__file__), "EditorScripts")


@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("level", ["tmp_level"])
@pytest.mark.usefixtures("automatic_process_killer")
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
class TestDebugger(object):
    @pytest.fixture(autouse=True)
    def setup_teardown(self, request, workspace, project, level):
        # Cleanup our temp level
        file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)

        def teardown():
            # Cleanup our temp level
            file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)

        request.addfinalizer(teardown)

    @pytest.mark.test_case_id("C2789148")
    @pytest.mark.SUITE_periodic
    @pytest.mark.dynveg_misc
    def test_Debugger_DebugCVarsWork(self, request, editor, level, workspace, launcher_platform):
        cfg_args = [level]

        expected_lines = [
            "Debugger_DebugCVarsWorks:  test started",
            "[Warning] Unknown command: veg_debugDumpReport",
            "[CONSOLE] Executing console command 'veg_debugRefreshAllAreas'",
            "Debugger_DebugCVarsWorks:  result=SUCCESS",
        ]

        hydra.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "Debugger_DebugCVarsWorks.py",
            expected_lines=expected_lines,
            cfg_args=cfg_args
        )
