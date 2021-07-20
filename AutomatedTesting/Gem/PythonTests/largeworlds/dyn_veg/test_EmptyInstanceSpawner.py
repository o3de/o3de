"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
import pytest
import logging

# Bail on the test if ly_test_tools doesn't exist.
pytest.importorskip('ly_test_tools')
import ly_test_tools.environment.file_system as file_system
import ly_test_tools._internal.pytest_plugin as internal_plugin
import editor_python_test_tools.hydra_test_utils as hydra

logger = logging.getLogger(__name__)
test_directory = os.path.join(os.path.dirname(__file__), 'EditorScripts')


@pytest.mark.parametrize('project', ['AutomatedTesting'])
@pytest.mark.parametrize('level', ['tmp_level'])
@pytest.mark.usefixtures("automatic_process_killer")
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
class TestEmptyInstanceSpawner(object):

    @pytest.fixture(autouse=True)
    def setup_teardown(self, request, workspace, project, level):
        def teardown():
            file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)
        request.addfinalizer(teardown)

        file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)

    @pytest.mark.test_case_id("C28851762")
    @pytest.mark.SUITE_main
    @pytest.mark.dynveg_area
    def test_EmptyInstanceSpawner_EmptySpawnerWorks(self, request, editor, level, launcher_platform):

        # Skip test if running against Debug build
        if "debug" in internal_plugin.build_directory:
            pytest.skip("Does not execute against debug builds.")

        cfg_args = [level]

        expected_lines = [
            "EmptyInstanceSpawner:  test started",
            "EmptyInstanceSpawner:  test finished",
            "EmptyInstanceSpawner:  result=SUCCESS"
        ]

        hydra.launch_and_validate_results(request, test_directory, editor, 'EmptyInstanceSpawner_EmptySpawnerWorks.py',
                                          expected_lines=expected_lines, cfg_args=cfg_args)
