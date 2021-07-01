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


@pytest.mark.parametrize('project', ['AutomatedTesting'])
@pytest.mark.parametrize('level', ['tmp_level'])
@pytest.mark.usefixtures("automatic_process_killer")
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
class TestGradientSampling(object):

    @pytest.fixture(autouse=True)
    def setup_teardown(self, request, workspace, project, level):
        def teardown():
            file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)
        request.addfinalizer(teardown)

        file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)

    @pytest.mark.test_case_id("C3526311")
    @pytest.mark.SUITE_periodic
    def test_GradientSampling_GradientReferencesAddRemoveSuccessfully(self, request, editor, level, launcher_platform):

        expected_lines = [
            "Entity has a Random Noise Gradient component",
            "Entity has a Gradient Transform Modifier component",
            "Entity has a Box Shape component",
            "Entity has a Dither Gradient Modifier component",
            "Gradient Generator is pinned to the Dither Gradient Modifier successfully",
            "Gradient Generator is cleared from the Dither Gradient Modifier successfully",
            "Entity has a Invert Gradient Modifier component",
            "Gradient Generator is pinned to the Invert Gradient Modifier successfully",
            "Gradient Generator is cleared from the Invert Gradient Modifier successfully",
            "Entity has a Levels Gradient Modifier component",
            "Gradient Generator is pinned to the Levels Gradient Modifier successfully",
            "Gradient Generator is cleared from the Levels Gradient Modifier successfully",
            "Entity has a Posterize Gradient Modifier component",
            "Gradient Generator is pinned to the Posterize Gradient Modifier successfully",
            "Gradient Generator is cleared from the Posterize Gradient Modifier successfully",
            "Entity has a Smooth-Step Gradient Modifier component",
            "Gradient Generator is pinned to the Smooth-Step Gradient Modifier successfully",
            "Gradient Generator is cleared from the Smooth-Step Gradient Modifier successfully",
            "Entity has a Threshold Gradient Modifier component",
            "Gradient Generator is pinned to the Threshold Gradient Modifier successfully",
            "Gradient Generator is cleared from the Threshold Gradient Modifier successfully",
        ]

        unexpected_lines = [
            "Failed to pin Gradient Generator to the Dither Gradient Modifier",
            "Failed to clear Gradient Generator from the Dither Gradient Modifier",
            "Failed to pin Gradient Generator to the Invert Gradient Modifier",
            "Failed to clear Gradient Generator from the Invert Gradient Modifier",
            "Failed to pin Gradient Generator to the Levels Gradient Modifier",
            "Failed to clear Gradient Generator from the Levels Gradient Modifier",
            "Failed to pin Gradient Generator to the Posterize Gradient Modifier",
            "Failed to clear Gradient Generator from the Posterize Gradient Modifier",
            "Failed to pin Gradient Generator to the Smooth-Step Gradient Modifier",
            "Failed to clear Gradient Generator from the Smooth-Step Gradient Modifier",
            "Failed to pin Gradient Generator to the Threshold Gradient Modifier",
            "Failed to clear Gradient Generator from the Threshold Gradient Modifier",
        ]

        hydra.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "GradientSampling_GradientReferencesAddRemoveSuccessfully.py",
            expected_lines,
            unexpected_lines,
            cfg_args=[level]
        )
