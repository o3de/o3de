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
import editor_python_test_tools.hydra_test_utils as hydra

logger = logging.getLogger(__name__)
test_directory = os.path.join(os.path.dirname(__file__), 'EditorScripts')


@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("level", ["tmp_level"])
@pytest.mark.usefixtures("automatic_process_killer")
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
class TestPositionModifier(object):

    @pytest.fixture(autouse=True)
    def setup_teardown(self, request, workspace, project, level):
        def teardown():
            file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)
        request.addfinalizer(teardown)

        file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)

    @pytest.mark.test_case_id("C4874099", "C4814461")
    @pytest.mark.SUITE_periodic
    @pytest.mark.dynveg_modifier
    def test_PositionModifier_ComponentAndOverrides_InstancesPlantAtSpecifiedOffsets(self, request, editor, level,
                                                                                     launcher_platform):

        expected_lines = [
            "'Instance Spawner' created",
            "Vegetation Position Modifier component was added to entity",
            "'Planting Surface' created",
            "Entity has a Constant Gradient component",
            "PositionModifierComponentAndOverrides_InstanceOffset:  result=SUCCESS",
        ]

        hydra.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "PositionModifier_ComponentAndOverrides_InstancesPlantAtSpecifiedOffsets.py",
            expected_lines,
            cfg_args=[level]
        )

    @pytest.mark.test_case_id("C4874100")
    @pytest.mark.SUITE_sandbox
    @pytest.mark.dynveg_modifier
    @pytest.mark.xfail  # LYN-3275
    def test_PositionModifier_AutoSnapToSurfaceWorks(self, request, editor, level, launcher_platform):

        expected_lines = [
            "'Instance Spawner' created",
            "'Planting Surface' created",
            "Instance Spawner Configuration|Position X|Range Min: SUCCESS",
            "Instance Spawner Configuration|Position X|Range Max: SUCCESS",
            "PositionModifier_AutoSnapToSurface:  result=SUCCESS"
        ]

        hydra.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "PositionModifier_AutoSnapToSurfaceWorks.py",
            expected_lines,
            cfg_args=[level]
        )
