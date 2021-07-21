"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
import pytest

# Bail on the test if ly_test_tools doesn't exist.
pytest.importorskip("ly_test_tools")

import editor_python_test_tools.hydra_test_utils as hydra
import ly_test_tools.environment.file_system as file_system

test_directory = os.path.join(os.path.dirname(__file__), "EditorScripts")


@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("level", ["tmp_level"])
@pytest.mark.usefixtures("automatic_process_killer")
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
class TestSurfaceMaskFilter(object):
    @pytest.fixture(autouse=True)
    def setup_teardown(self, request, workspace, project, level):
        def teardown():
            # delete temp level
            file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)

        # Setup - add the teardown finalizer
        request.addfinalizer(teardown)

        # Make sure the temp level doesn't already exist
        file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)

    # Simple validation test to ensure that SurfaceTag can be created, set to a value, and compared to another SurfaceTag.
    @pytest.mark.SUITE_periodic
    @pytest.mark.dynveg_filter
    def test_SurfaceMaskFilter_BasicSurfaceTagCreation(self, request, level, editor, launcher_platform):

        expected_lines = [
            "SurfaceTag test started",
            "SurfaceTag equal tag comparison is True expected True",
            "SurfaceTag not equal tag comparison is False expected False",
            "SurfaceTag test finished",
            "TestSurfaceMaskFilter_BasicSurfaceTagCreation:  result=SUCCESS"
        ]

        hydra.launch_and_validate_results(
            request,
            test_directory,
            editor,
            'SurfaceMaskFilter_BasicSurfaceTagCreation.py',
            expected_lines=expected_lines,
            cfg_args=[level]
        )

    @pytest.mark.test_case_id("C2561342")
    @pytest.mark.SUITE_periodic
    @pytest.mark.dynveg_filter
    def test_SurfaceMaskFilter_ExclusiveSurfaceTags_Function(self, request, editor, level, launcher_platform):

        expected_lines = [
            "'Instance Spawner' created",
            "Instance Spawner Box Shape|Box Configuration|Dimensions: SUCCESS",
            "Instance Spawner Configuration|Embedded Assets|[0]: SUCCESS",
            "'Surface Entity 1' created",
            "Surface Entity 1 Box Shape|Box Configuration|Dimensions: SUCCESS",
            "Surface Entity 1 Configuration|Generated Tags: SUCCESS",
            "'Surface Entity 2' created",
            "Surface Entity 2 Box Shape|Box Configuration|Dimensions: SUCCESS",
            "Surface Entity 2 Configuration|Generated Tags: SUCCESS",
            "SurfaceMaskFilter_ExclusionList:  Expected 39 instances - Found 39 instances",
            "Instance Spawner Configuration|Exclusion|Weight Max: SUCCESS",
            "SurfaceMaskFilter_ExclusionList:  Expected 169 instances - Found 169 instances",
            "SurfaceMaskFilter_ExclusionList:  result=SUCCESS"
        ]

        unexpected_lines = ["Failed to add an Exclusive surface mask filter of terrainHole"]

        hydra.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "SurfaceMaskFilter_ExclusionList.py",
            expected_lines,
            unexpected_lines,
            cfg_args=[level]
        )

    @pytest.mark.test_case_id("C2561341")
    @pytest.mark.SUITE_periodic
    @pytest.mark.dynveg_filter
    def test_SurfaceMaskFilter_InclusiveSurfaceTags_Function(self, request, editor, level, launcher_platform):

        expected_lines = [
            "'Instance Spawner' created",
            "Instance Spawner Box Shape|Box Configuration|Dimensions: SUCCESS",
            "Instance Spawner Configuration|Embedded Assets|[0]: SUCCESS",
            "'Surface Entity 1' created",
            "Surface Entity 1 Box Shape|Box Configuration|Dimensions: SUCCESS",
            "Surface Entity 1 Configuration|Generated Tags: SUCCESS",
            "'Surface Entity 2' created",
            "Surface Entity 2 Box Shape|Box Configuration|Dimensions: SUCCESS",
            "Surface Entity 2 Configuration|Generated Tags: SUCCESS",
            "SurfaceMaskFilter_InclusionList:  Expected 130 instances - Found 130 instances",
            "Instance Spawner Configuration|Inclusion|Weight Max: SUCCESS",
            "SurfaceMaskFilter_InclusionList:  Expected 0 instances - Found 0 instances",
            "SurfaceMaskFilter_InclusionList:  result=SUCCESS"
        ]

        unexpected_lines = ["Failed to add an Inclusive surface mask filter of terrainHole"]

        hydra.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "SurfaceMaskFilter_InclusionList.py",
            expected_lines,
            unexpected_lines,
            cfg_args=[level]
        )

    @pytest.mark.test_case_id("C3711666")
    @pytest.mark.SUITE_periodic
    @pytest.mark.dynveg_filter
    def test_SurfaceMaskFilterOverrides_MultipleDescriptorOverridesPlantAsExpected(self, request, editor, level,
                                                                                   launcher_platform):

        expected_lines = [
            "'Instance Spawner' created",
            "'Surface Entity A' created",
            "'Surface Entity B' created",
            "'Surface Entity C' created",
            "instance count validation: True (found=725, expected=725)",
            "instance count validation: True (found=400, expected=400)",
            "instance count validation: True (found=225, expected=225)",
            "instance count validation: True (found=100, expected=100)",
            "SurfaceMaskFilter_MultipleDescriptorOverrides:  result=SUCCESS"
        ]

        hydra.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "SurfaceMaskFilterOverrides_MultipleDescriptorOverridesPlantAsExpected.py",
            expected_lines,
            cfg_args=[level]
        )
