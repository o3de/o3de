"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

"""
C5747383: Vegetation areas with a higher Layer Priority plant over those with a lower Layer Priority
C4762382: Vegetation areas with a higher Sub Priority plant over those with a lower Sub Priority
"""

import os
import pytest
import logging

# Bail on the test if ly_test_tools doesn't exist.
pytest.importorskip('ly_test_tools')
import ly_test_tools.environment.file_system as file_system
import editor_python_test_tools.hydra_test_utils as hydra

logger = logging.getLogger(__name__)
test_directory = os.path.join(os.path.dirname(__file__), 'EditorScripts')


@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("level", ["tmp_level"])
@pytest.mark.usefixtures("automatic_process_killer")
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
class TestInstanceSpawnerPriority(object):

    @pytest.fixture(autouse=True)
    def setup_teardown(self, request, workspace, project, level):
        def teardown():
            file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)
        request.addfinalizer(teardown)

        file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)

    @pytest.mark.test_case_id("C5747383", "C4762382")
    @pytest.mark.SUITE_periodic
    @pytest.mark.dynveg_misc
    def test_InstanceSpawnerPriority_LayerAndSubPriority_HigherValuesPlantOverLower(self, request, editor, level,
                                                                                    launcher_platform):

        expected_lines = [
            "'Instance Spawner' created",
            "'Instance Blocker' created",
            "'Planting Surface' created",
            "Instance Blocker Configuration|Layer Priority: SUCCESS",
            "Instance Spawner Configuration|Sub Priority: SUCCESS",
            "Instance Blocker Configuration|Sub Priority: SUCCESS",
            "InstanceSpawnerPriority:  result=SUCCESS",
        ]

        hydra.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "InstanceSpawnerPriority_LayerAndSubPriority.py",
            expected_lines,
            cfg_args=[level]
        )
