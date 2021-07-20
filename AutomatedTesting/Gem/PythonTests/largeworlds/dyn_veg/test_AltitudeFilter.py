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
class TestAltitudeFilter(object):
    
    @pytest.fixture(autouse=True)
    def setup_teardown(self, request, workspace, project, level):
        def teardown():
            file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)
        request.addfinalizer(teardown)

        file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)

    @pytest.mark.test_case_id('C4814463', 'C4847477')
    @pytest.mark.SUITE_periodic
    @pytest.mark.dynveg_filter
    def test_AltitudeFilter_ComponentAndOverrides_InstancesPlantAtSpecifiedAltitude(self, request, editor, level,
                                                                                    launcher_platform):

        expected_lines = [
            "'Instance Spawner' created",
            "'Planting Surface' created",
            "'Planting Surface Elevated' created",
            "instance count validation: True (found=3200, expected=3200)",
            "instance count validation: True (found=1600, expected=1600)",
            "instance count validation: True (found=400, expected=400)",
            "AltitudeFilterComponentAndOverrides:  result=SUCCESS"
        ]

        hydra.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "AltitudeFilter_ComponentAndOverrides_InstancesPlantAtSpecifiedAltitude.py",
            expected_lines,
            cfg_args=[level]
        )

    @pytest.mark.test_case_id("C4847476")
    @pytest.mark.SUITE_periodic
    @pytest.mark.dynveg_filter
    def test_AltitudeFilter_ShapeSample_InstancesPlantAtSpecifiedAltitude(self, request, editor, level,
                                                                          launcher_platform):

        expected_lines = [
            "'Instance Spawner' created",
            "'Planting Surface' created",
            "'Planting Surface Elevated' created",
            "instance count validation: True (found=800, expected=800)",
            "'Shape Sampler' created",
            "instance count validation: True (found=400, expected=400)",
            "AltitudeFilterShapeSample:  result=SUCCESS"
        ]

        hydra.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "AltitudeFilter_ShapeSample_InstancesPlantAtSpecifiedAltitude.py",
            expected_lines,
            cfg_args=[level]
        )

    @pytest.mark.test_case_id("C4847478")
    @pytest.mark.SUITE_periodic
    @pytest.mark.dynveg_filter
    @pytest.mark.xfail  # LYN-3275
    def test_AltitudeFilter_FilterStageToggle(self, request, editor, level, workspace, launcher_platform):
        cfg_args = [level]

        expected_lines = [
            "AltitudeFilter_FilterStageToggle:  test started",
            "AltitudeFilter_FilterStageToggle:  Vegetation instances count equal to expected value for PREPROCESS filter stage: True",
            "AltitudeFilter_FilterStageToggle:  Vegetation instances count equal to expected value for POSTPROCESS filter stage: True",
            "AltitudeFilter_FilterStageToggle:  result=SUCCESS",
        ]

        unexpected_lines = [
            "AltitudeFilter_FilterStageToggle:  Vegetation instances count equal to expected value for PREPROCESS filter stage: False",
            "AltitudeFilter_FilterStageToggle:  Vegetation instances count equal to expected value for POSTPROCESS filter stage: False",
        ]

        hydra.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "AltitudeFilter_FilterStageToggle.py",
            expected_lines=expected_lines,
            unexpected_lines=unexpected_lines,
            cfg_args=cfg_args
        )
