"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


"""
Tests that the Gradient Generator components are incompatible with Vegetation Area components
"""

import os
import pytest
pytest.importorskip('ly_test_tools')

import ly_test_tools.environment.file_system as file_system
import editor_python_test_tools.hydra_test_utils as hydra

test_directory = os.path.join(os.path.dirname(__file__), 'EditorScripts')

gradient_generators = [
    'Altitude Gradient',
    'Constant Gradient',
    'FastNoise Gradient',
    'Image Gradient',
    'Perlin Noise Gradient',
    'Random Noise Gradient',
    'Shape Falloff Gradient',
    'Slope Gradient',
    'Surface Mask Gradient'
]

gradient_modifiers = [
    'Dither Gradient Modifier',
    'Gradient Mixer',
    'Invert Gradient Modifier',
    'Levels Gradient Modifier',
    'Posterize Gradient Modifier',
    'Smooth-Step Gradient Modifier',
    'Threshold Gradient Modifier'
]

vegetation_areas = [
    'Vegetation Layer Spawner',
    'Vegetation Layer Blender',
    'Vegetation Layer Blocker',
    'Vegetation Layer Blocker (Mesh)'
]

all_gradients = gradient_modifiers + gradient_generators


@pytest.mark.parametrize('project', ['AutomatedTesting'])
@pytest.mark.parametrize('level', ['tmp_level'])
@pytest.mark.usefixtures("automatic_process_killer")
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
class TestGradientIncompatibilities(object):
    @pytest.fixture(autouse=True)
    def setup_teardown(self, request, workspace, project, level):
        def teardown():
            file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)

        request.addfinalizer(teardown)
    
    @pytest.mark.test_case_id('C2691648', 'C2691649', 'C2691650', 'C2691651',
                              'C2691653', 'C2691656', 'C2691657', 'C2691658',
                              'C2691647', 'C2691655')
    @pytest.mark.SUITE_periodic
    def test_GradientGenerators_Incompatibilities(self, request, editor, level, launcher_platform):
        cfg_args = [level]

        expected_lines = []
        for gradient_generator in gradient_generators:
            for vegetation_area in vegetation_areas:
                expected_lines.append(f"{gradient_generator} is disabled before removing {vegetation_area} component")
                expected_lines.append(f"{gradient_generator} is enabled after removing {vegetation_area} component")
        expected_lines.append("GradientGeneratorIncompatibilities:  result=SUCCESS")
        hydra.launch_and_validate_results(request, test_directory, editor,
                                          'GradientGenerators_Incompatibilities.py',
                                          expected_lines=expected_lines, cfg_args=cfg_args)

    @pytest.mark.test_case_id('C3416464', 'C3416546', 'C3961318', 'C3961319',
                              'C3961323', 'C3961324', 'C3980656', 'C3980657',
                              'C3980661', 'C3980662', 'C3980666', 'C3980667',
                              'C2691652')
    @pytest.mark.SUITE_periodic
    def test_GradientModifiers_Incompatibilities(self, request, editor, level, launcher_platform):
        cfg_args = [level]

        expected_lines = []
        for gradient_modifier in gradient_modifiers:
            for vegetation_area in vegetation_areas:
                expected_lines.append(f"{gradient_modifier} is disabled before removing {vegetation_area} component")
                expected_lines.append(f"{gradient_modifier} is enabled after removing {vegetation_area} component")

            for conflicting_gradient in all_gradients:
                expected_lines.append(f"{gradient_modifier} is disabled before removing {conflicting_gradient} component")
                expected_lines.append(f"{gradient_modifier} is enabled after removing {conflicting_gradient} component")
        expected_lines.append("GradientModifiersIncompatibilities:  result=SUCCESS")
        hydra.launch_and_validate_results(request, test_directory, editor,
                                          'GradientModifiers_Incompatibilities.py',
                                          expected_lines=expected_lines, cfg_args=cfg_args)
