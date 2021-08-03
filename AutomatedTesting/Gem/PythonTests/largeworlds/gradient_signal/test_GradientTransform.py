"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


"""
Tests that the Gradient Transform Modifier component isn't enabled unless it has a component on
the same Entity that provides the ShapeService (e.g. box shape, or reference shape)
"""

import os
import pytest

# Bail on the test if ly_test_tools doesn't exist.
pytest.importorskip('ly_test_tools')
import ly_test_tools.environment.file_system as file_system
import editor_python_test_tools.hydra_test_utils as hydra

test_directory = os.path.join(os.path.dirname(__file__), 'EditorScripts')


@pytest.mark.parametrize('project', ['AutomatedTesting'])
@pytest.mark.parametrize('level', ['tmp_level'])
@pytest.mark.usefixtures("automatic_process_killer")
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
class TestGradientTransformRequiresShape(object):

    @pytest.fixture(autouse=True)
    def setup_teardown(self, request, workspace, project, level):
        def teardown():
            file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)

        request.addfinalizer(teardown)
        file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)

    @pytest.mark.test_case_id('C3430289')
    @pytest.mark.SUITE_periodic
    def test_GradientTransform_RequiresShape(self, request, editor, level, launcher_platform):

        expected_lines = [
            "Gradient Transform Modifier component was added to entity, but the component is disabled",
            "Gradient Transform component is not active without a Shape component on the Entity",
            "Box Shape component was added to entity",
            "Gradient Transform Modifier component is active now that the Entity has a Shape",
            "GradientTransformRequiresShape:  result=SUCCESS"
        ]

        hydra.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "GradientTransform_RequiresShape.py",
            expected_lines,
            cfg_args=[level]
        )

    @pytest.mark.test_case_id("C3430292")
    @pytest.mark.SUITE_periodic
    def test_GradientTransform_FrequencyZoomCanBeSetBeyondSliderRange(self, request, editor, level, launcher_platform):

        expected_lines = [
            "Entity Created",
            "Entity has a Random Noise Gradient component",
            "Entity has a Gradient Transform Modifier component",
            "Entity has a Box Shape component",
            "Components added to the entity",
            "entity Configuration|Frequency Zoom: SUCCESS",
            "Frequency Zoom is equal to expected value",
        ]

        unexpected_lines = ["Frequency Zoom is not equal to expected value"]

        hydra.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "GradientTransform_FrequencyZoomCanBeSetBeyondSliderRange.py",
            expected_lines,
            unexpected_lines=unexpected_lines,
            cfg_args=[level]
        )

    @pytest.mark.test_case_id("C3430297")
    @pytest.mark.SUITE_periodic
    def test_GradientTransform_ComponentIncompatibleWithSpawners(self, request, editor, launcher_platform, level):
        # C3430297: Component cannot be active on the same Entity as an active Vegetation Layer Spawner
        expected_lines = [
            "Entity has a Gradient Transform Modifier component",
            "Entity has a Box Shape component",
            "New Entity Created",
            "Gradient Transform Modifier is Enabled",
            "Box Shape is Enabled",
            "Entity has a Vegetation Layer Spawner component",
            "Vegetation Layer Spawner is incompatible and disabled",
            "GradientTransform_ComponentIncompatibleWithSpawners:  result=SUCCESS"
        ]

        unexpected_lines = [
            "Gradient Transform Modifier is Disabled. But It should be Enabled in an Entity",
            "Box Shape is Disabled. But It should be Enabled in an Entity",
            "Vegetation Layer Spawner is compatible and enabled. But It should be Incompatible and disabled",
        ]

        hydra.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "GradientTransform_ComponentIncompatibleWithSpawners.py",
            expected_lines,
            unexpected_lines,
            cfg_args=[level]
        )

    @pytest.mark.test_case_id("C4753767")
    @pytest.mark.SUITE_periodic
    def test_GradientTransform_ComponentIncompatibleWithExpectedGradients(self, request, editor, launcher_platform, level):
        expected_lines = [
            "Entity has a Gradient Transform Modifier component",
            "Entity has a Box Shape component",
            "New Entity Created",
            "Gradient Transform Modifier is Enabled",
            "Box Shape is Enabled",
            "Entity has a Constant Gradient component",
            "Entity has a Altitude Gradient component",
            "Entity has a Gradient Mixer component",
            "Entity has a Reference Gradient component",
            "Entity has a Shape Falloff Gradient component",
            "Entity has a Slope Gradient component",
            "Entity has a Surface Mask Gradient component",
            "All newly added components are incompatible and disabled",
            "GradientTransform_ComponentIncompatibleWithExpectedGradients:  result=SUCCESS"
        ]

        unexpected_lines = [
            "Gradient Transform Modifier is disabled, but it should be enabled",
            "Box Shape is disabled, but it should be enabled",
            "Constant Gradient is enabled, but should be disabled",
            "Altitude Gradient is enabled, but should be disabled",
            "Gradient Mixer is enabled, but should be disabled",
            "Reference Gradient is enabled, but should be disabled",
            "Shape Falloff Gradient is enabled, but should be disabled",
            "Slope Gradient is enabled, but should be disabled",
            "Surface Mask Gradient component is enabled, but should be disabled",
        ]

        hydra.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "GradientTransform_ComponentIncompatibleWithExpectedGradients.py",
            expected_lines,
            unexpected_lines,
            cfg_args=[level]
        )
