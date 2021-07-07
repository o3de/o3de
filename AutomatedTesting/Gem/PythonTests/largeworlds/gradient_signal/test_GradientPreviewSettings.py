"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
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
class TestGradientPreviewSettings(object):

    @pytest.fixture(autouse=True)
    def setup_teardown(self, request, workspace, project, level):
        def teardown():
            file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)
        request.addfinalizer(teardown)

        file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)

    @pytest.mark.test_case_id('C3980668', 'C2676825', 'C2676828', 'C2676822', 'C3416547', 'C3961320', 'C3961325',
                              'C3980658', 'C3980663')
    @pytest.mark.SUITE_periodic
    def test_GradientPreviewSettings_DefaultPinnedEntityIsSelf(self, request, editor, level, launcher_platform):

        expected_lines = [
            "Perlin Noise Gradient has Preview pinned to own Entity result: SUCCESS",
            "Random Noise Gradient has Preview pinned to own Entity result: SUCCESS",
            "FastNoise Gradient has Preview pinned to own Entity result: SUCCESS",
            "Dither Gradient Modifier has Preview pinned to own Entity result: SUCCESS",
            "Invert Gradient Modifier has Preview pinned to own Entity result: SUCCESS",
            "Levels Gradient Modifier has Preview pinned to own Entity result: SUCCESS",
            "Posterize Gradient Modifier has Preview pinned to own Entity result: SUCCESS",
            "Smooth-Step Gradient Modifier has Preview pinned to own Entity result: SUCCESS",
            "Threshold Gradient Modifier has Preview pinned to own Entity result: SUCCESS",
            "GradientPreviewSettings_DefaultPinnedEntity:  result=SUCCESS"
        ]

        hydra.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "GradientPreviewSettings_DefaultPinnedEntityIsSelf.py",
            expected_lines,
            cfg_args=[level]
        )

    @pytest.mark.test_case_id("C2676829", "C3961326", "C3980659", "C3980664", "C3980669", "C3416548", "C2676823",
                              "C3961321", "C2676826")
    @pytest.mark.SUITE_periodic
    def test_GradientPreviewSettings_ClearingPinnedEntitySetsPreviewToOrigin(self, request, editor, level,
                                                                             launcher_platform):

        expected_lines = [
            "Random Noise Gradient entity Created",
            "Entity has a Random Noise Gradient component",
            "Entity has a Gradient Transform Modifier component",
            "Entity has a Box Shape component",
            "Random Noise Gradient Preview Settings|Pin Preview to Shape: SUCCESS",
            "Random Noise Gradient --- Preview Position set to world origin",
            "Random Noise Gradient --- Preview Size set to (1, 1, 1)",
            "Levels Gradient Modifier entity Created",
            "Entity has a Levels Gradient Modifier component",
            "Levels Gradient Modifier Preview Settings|Pin Preview to Shape: SUCCESS",
            "Levels Gradient Modifier --- Preview Position set to world origin",
            "Posterize Gradient Modifier entity Created",
            "Entity has a Posterize Gradient Modifier component",
            "Posterize Gradient Modifier Preview Settings|Pin Preview to Shape: SUCCESS",
            "Posterize Gradient Modifier --- Preview Position set to world origin",
            "Smooth-Step Gradient Modifier entity Created",
            "Entity has a Smooth-Step Gradient Modifier component",
            "Smooth-Step Gradient Modifier Preview Settings|Pin Preview to Shape: SUCCESS",
            "Smooth-Step Gradient Modifier --- Preview Position set to world origin",
            "Threshold Gradient Modifier entity Created",
            "Entity has a Threshold Gradient Modifier component",
            "Threshold Gradient Modifier Preview Settings|Pin Preview to Shape: SUCCESS",
            "Threshold Gradient Modifier --- Preview Position set to world origin",
            "FastNoise Gradient entity Created",
            "Entity has a FastNoise Gradient component",
            "FastNoise Gradient Preview Settings|Pin Preview to Shape: SUCCESS",
            "FastNoise Gradient --- Preview Position set to world origin",
            "FastNoise Gradient --- Preview Size set to (1, 1, 1)",
            "Dither Gradient Modifier entity Created",
            "Entity has a Dither Gradient Modifier component",
            "Dither Gradient Modifier Preview Settings|Pin Preview to Shape: SUCCESS",
            "Dither Gradient Modifier --- Preview Position set to world origin",
            "Dither Gradient Modifier --- Preview Size set to (1, 1, 1)",
            "Invert Gradient Modifier entity Created",
            "Entity has a Invert Gradient Modifier component",
            "Invert Gradient Modifier Preview Settings|Pin Preview to Shape: SUCCESS",
            "Invert Gradient Modifier --- Preview Position set to world origin",
            "Perlin Noise Gradient entity Created",
            "Entity has a Perlin Noise Gradient component",
            "Perlin Noise Gradient Preview Settings|Pin Preview to Shape: SUCCESS",
            "Perlin Noise Gradient --- Preview Position set to world origin",
            "Perlin Noise Gradient --- Preview Size set to (1, 1, 1)",
        ]

        hydra.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "GradientPreviewSettings_ClearingPinnedEntitySetsPreviewToOrigin.py",
            expected_lines,
            cfg_args=[level]
        )
