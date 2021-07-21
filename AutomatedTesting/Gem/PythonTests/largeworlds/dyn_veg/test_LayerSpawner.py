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
class TestLayerSpawner(object):
    @pytest.fixture(autouse=True)
    def setup_teardown(self, request, workspace, project, level):
        # Cleanup our temp level
        file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)

        def teardown():
            # Cleanup our temp level
            file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)

        request.addfinalizer(teardown)

    @pytest.mark.test_case_id("C4762381")
    @pytest.mark.SUITE_periodic
    @pytest.mark.dynveg_misc
    def test_LayerSpawner_InheritBehaviorFlag(self, request, editor, level, workspace, launcher_platform):

        expected_lines = [
            "LayerSpawner_InheritBehavior:  test started",
            "LayerSpawner_InheritBehavior:  Vegetation is not planted when Inherit Behavior flag is checked: True",
            "LayerSpawner_InheritBehavior:  Vegetation plant when Inherit Behavior flag is unchecked: True",
            "LayerSpawner_InheritBehavior:  result=SUCCESS",
        ]

        hydra.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "LayerSpawner_InheritBehaviorFlag.py",
            expected_lines=expected_lines,
            cfg_args=[level]
        )

    @pytest.mark.test_case_id("C2802020")
    @pytest.mark.SUITE_periodic
    @pytest.mark.dynveg_misc
    def test_LayerSpawner_InstancesPlantInAllSupportedShapes(self, request, editor, level, launcher_platform):

        expected_lines = [
            "'Instance Spawner' created",
            "'Surface Entity' created",
            "Entity has a Vegetation Reference Shape component",
            "Entity has a Box Shape component",
            "box Box Shape|Box Configuration|Dimensions: SUCCESS",
            "Entity has a Capsule Shape component",
            "capsule Capsule Shape|Capsule Configuration|Height: SUCCESS",
            "capsule Capsule Shape|Capsule Configuration|Radius: SUCCESS",
            "Entity has a Tube Shape component",
            "Entity has a Spline component",
            "Entity has a Sphere Shape component",
            "sphere Sphere Shape|Sphere Configuration|Radius: SUCCESS",
            "Entity has a Cylinder Shape component",
            "cylinder Cylinder Shape|Cylinder Configuration|Radius: SUCCESS",
            "cylinder Cylinder Shape|Cylinder Configuration|Height: SUCCESS",
            "Entity has a Polygon Prism Shape component",
            "Entity has a Compound Shape component",
            "Compound Configuration|Child Shape Entities|[0]: SUCCESS",
            "Compound Configuration|Child Shape Entities|[1]: SUCCESS",
            "Compound Configuration|Child Shape Entities|[2]: SUCCESS",
            "Compound Configuration|Child Shape Entities|[3]: SUCCESS",
            "Compound Configuration|Child Shape Entities|[4]: SUCCESS",
            "Compound Configuration|Child Shape Entities|[5]: SUCCESS",
            "Instance Spawner Configuration|Shape Entity Id: SUCCESS",
            "TestLayerSpawner_AllShapesPlant:  result=SUCCESS",
        ]

        hydra.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "LayerSpawner_InstancesPlantInAllSupportedShapes.py",
            expected_lines,
            cfg_args=[level]
        )

    @pytest.mark.test_case_id("C4765973")
    @pytest.mark.SUITE_periodic
    @pytest.mark.dynveg_misc
    @pytest.mark.xfail      # LYN-3275
    def test_LayerSpawner_FilterStageToggle(self, request, editor, level, workspace, launcher_platform):

        expected_lines = [
            "LayerSpawner_FilterStageToggle:  test started",
            "LayerSpawner_FilterStageToggle:  Preprocess filter stage vegetation instance count is as expected: True",
            "LayerSpawner_FilterStageToggle:  Postprocess filter vegetation instance stage count is as expected: True",
            "LayerSpawner_FilterStageToggle:  result=SUCCESS",
        ]

        hydra.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "LayerSpawner_FilterStageToggle.py",
            expected_lines=expected_lines,
            cfg_args=[level]
        )

    @pytest.mark.test_case_id("C30000751")
    @pytest.mark.SUITE_sandbox
    @pytest.mark.dynveg_misc
    @pytest.mark.skip   # ATOM-14828
    def test_LayerSpawner_InstancesRefreshUsingCorrectViewportCamera(self, request, editor, level, launcher_platform):

        expected_lines = [
            "LayerSpawner_InstanceCameraRefresh:  test started",
            "LayerSpawner_InstanceCameraRefresh:  test finished",
            "LayerSpawner_InstanceCameraRefresh:  result=SUCCESS"
        ]

        hydra.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "LayerSpawner_InstancesRefreshUsingCorrectViewportCamera.py",
            expected_lines,
            cfg_args=[level]
        )
