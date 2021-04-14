"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

import os
import pytest
import logging

# Bail on the test if ly_test_tools doesn't exist.
pytest.importorskip("ly_test_tools")
import ly_test_tools.environment.file_system as file_system
import automatedtesting_shared.hydra_test_utils as hydra

logger = logging.getLogger(__name__)
test_directory = os.path.join(os.path.dirname(__file__), "EditorScripts")


@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("level", ["tmp_level"])
@pytest.mark.usefixtures("automatic_process_killer")
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
class TestSlopeFilter(object):
    @pytest.fixture(autouse=True)
    def setup_teardown(self, request, workspace, project, level):
        # Cleanup our temp level
        file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)

        def teardown():
            # Cleanup our temp level
            file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)
        request.addfinalizer(teardown)

    @pytest.mark.test_case_id("C4874097")
    @pytest.mark.SUITE_periodic
    def test_SlopeFilter_FilterStageToggle(self, request, editor, level, workspace, launcher_platform):
        cfg_args = [level]

        expected_lines = [
            "SlopeFilter_FilterStageToggle:  test started",
            "SlopeFilter_FilterStageToggle:  Vegetation plant only in the areas where the Box overlaps with the vegetation area's boundaries: True",
            "SlopeFilter_FilterStageToggle:  Vegetation plant only in the areas where the Cylinder overlaps with the vegetation area's boundaries: True",
            "SlopeFilter_FilterStageToggle:  Vegetation instances count equal to expected value for PREPROCESS filter stage: True",
            "SlopeFilter_FilterStageToggle:  Vegetation instances count equal to expected value for POSTPROCESS filter stage: True",
            "SlopeFilter_FilterStageToggle:  result=SUCCESS",
        ]

        unexpected_lines = [
            "SlopeFilter_FilterStageToggle:  Vegetation plant only in the areas where the Box overlaps with the vegetation area's boundaries: False",
            "SlopeFilter_FilterStageToggle:  Vegetation plant only in the areas where the Cylinder overlaps with the vegetation area's boundaries: False",
            "SlopeFilter_FilterStageToggle:  Vegetation instances count equal to expected value for PREPROCESS filter stage: False",
            "SlopeFilter_FilterStageToggle:  Vegetation instances count equal to expected value for POSTPROCESS filter stage: False",
        ]

        hydra.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "SlopeFilter_FilterStageToggle.py",
            expected_lines=expected_lines,
            unexpected_lines=unexpected_lines,
            cfg_args=cfg_args
        )

    @pytest.mark.test_case_id("C4814464", "C4874096")
    @pytest.mark.SUITE_periodic
    @pytest.mark.skip  # LYN-2211
    def test_SlopeFilter_ComponentAndOverrides_InstancesPlantOnValidSlopes(self, request, editor, level,
                                                                           launcher_platform):

        expected_lines = [
            "'Instance Spawner' created",
            "'Planting Surface' created",
            "'Sloped Planting Surface' created",
            "instance count validation: True (found=1720, expected=1720)",
            "Instance Spawner Configuration|Slope Min: SUCCESS",
            "Instance Spawner Configuration|Slope Max: SUCCESS",
            "instance count validation: True (found=44, expected=44)",
            "Instance Spawner Configuration|Embedded Assets|[0]|Slope Filter|Min: SUCCESS",
            "Instance Spawner Configuration|Embedded Assets|[0]|Slope Filter|Max: SUCCESS",
            "instance count validation: True (found=16, expected=16)",
            "SlopeFilter_InstancesPlantOnValidSlope:  result=SUCCESS"
        ]

        hydra.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "SlopeFilter_ComponentAndOverrides_InstancesPlantOnValidSlope.py",
            expected_lines,
            cfg_args=[level]
        )
