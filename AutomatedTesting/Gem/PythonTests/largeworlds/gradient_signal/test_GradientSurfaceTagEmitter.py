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
class TestGradientSurfaceTagEmitter(object):

    @pytest.fixture(autouse=True)
    def setup_teardown(self, request, workspace, project, level):
        # Cleanup temp level before and after test runs
        def teardown():
            file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)
        request.addfinalizer(teardown)

        file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)

    @pytest.mark.test_case_id("C3297302")
    @pytest.mark.SUITE_periodic
    def test_GradientSurfaceTagEmitter_ComponentDependencies(self, request, editor, level, workspace,
                                                             launcher_platform):
        cfg_args = [level]

        expected_lines = [
            "GradientSurfaceTagEmitter_ComponentDependencies:  test started",
            "GradientSurfaceTagEmitter_ComponentDependencies:  Gradient Surface Tag Emitter is Disabled",
            "GradientSurfaceTagEmitter_ComponentDependencies:  Dither Gradient Modifier and Gradient Surface Tag Emitter are enabled",
            "GradientSurfaceTagEmitter_ComponentDependencies:  Gradient Mixer and Gradient Surface Tag Emitter are enabled",
            "GradientSurfaceTagEmitter_ComponentDependencies:  Invert Gradient Modifier and Gradient Surface Tag Emitter are enabled",
            "GradientSurfaceTagEmitter_ComponentDependencies:  Levels Gradient Modifier and Gradient Surface Tag Emitter are enabled",
            "GradientSurfaceTagEmitter_ComponentDependencies:  Posterize Gradient Modifier and Gradient Surface Tag Emitter are enabled",
            "GradientSurfaceTagEmitter_ComponentDependencies:  Smooth-Step Gradient Modifier and Gradient Surface Tag Emitter are enabled",
            "GradientSurfaceTagEmitter_ComponentDependencies:  Threshold Gradient Modifier and Gradient Surface Tag Emitter are enabled",
            "GradientSurfaceTagEmitter_ComponentDependencies:  Altitude Gradient and Gradient Surface Tag Emitter are enabled",
            "GradientSurfaceTagEmitter_ComponentDependencies:  Constant Gradient and Gradient Surface Tag Emitter are enabled",
            "GradientSurfaceTagEmitter_ComponentDependencies:  FastNoise Gradient and Gradient Surface Tag Emitter are enabled",
            "GradientSurfaceTagEmitter_ComponentDependencies:  Image Gradient and Gradient Surface Tag Emitter are enabled",
            "GradientSurfaceTagEmitter_ComponentDependencies:  Perlin Noise Gradient and Gradient Surface Tag Emitter are enabled",
            "GradientSurfaceTagEmitter_ComponentDependencies:  Random Noise Gradient and Gradient Surface Tag Emitter are enabled",
            "GradientSurfaceTagEmitter_ComponentDependencies:  Reference Gradient and Gradient Surface Tag Emitter are enabled",
            "GradientSurfaceTagEmitter_ComponentDependencies:  Shape Falloff Gradient and Gradient Surface Tag Emitter are enabled",
            "GradientSurfaceTagEmitter_ComponentDependencies:  Slope Gradient and Gradient Surface Tag Emitter are enabled",
            "GradientSurfaceTagEmitter_ComponentDependencies:  Surface Mask Gradient and Gradient Surface Tag Emitter are enabled",
            "GradientSurfaceTagEmitter_ComponentDependencies:  result=SUCCESS",
        ]

        unexpected_lines = [
            "GradientSurfaceTagEmitter_ComponentDependencies:  Gradient Surface Tag Emitter is Enabled, but should be Disabled without dependencies met",
            "GradientSurfaceTagEmitter_ComponentDependencies:  Dither Gradient Modifier and Gradient Surface Tag Emitter are disabled",
            "GradientSurfaceTagEmitter_ComponentDependencies:  Gradient Mixer and Gradient Surface Tag Emitter are disabled",
            "GradientSurfaceTagEmitter_ComponentDependencies:  Invert Gradient Modifier and Gradient Surface Tag Emitter are disabled",
            "GradientSurfaceTagEmitter_ComponentDependencies:  Levels Gradient Modifier and Gradient Surface Tag Emitter are disabled",
            "GradientSurfaceTagEmitter_ComponentDependencies:  Posterize Gradient Modifier and Gradient Surface Tag Emitter are disabled",
            "GradientSurfaceTagEmitter_ComponentDependencies:  Smooth-Step Gradient Modifier and Gradient Surface Tag Emitter are disabled",
            "GradientSurfaceTagEmitter_ComponentDependencies:  Threshold Gradient Modifier and Gradient Surface Tag Emitter are disabled",
            "GradientSurfaceTagEmitter_ComponentDependencies:  Altitude Gradient and Gradient Surface Tag Emitter are disabled",
            "GradientSurfaceTagEmitter_ComponentDependencies:  Constant Gradient and Gradient Surface Tag Emitter are disabled",
            "GradientSurfaceTagEmitter_ComponentDependencies:  FastNoise Gradient and Gradient Surface Tag Emitter are disabled",
            "GradientSurfaceTagEmitter_ComponentDependencies:  Image Gradient and Gradient Surface Tag Emitter are disabled",
            "GradientSurfaceTagEmitter_ComponentDependencies:  Perlin Noise Gradient and Gradient Surface Tag Emitter are disabled",
            "GradientSurfaceTagEmitter_ComponentDependencies:  Random Noise Gradient and Gradient Surface Tag Emitter are disabled",
            "GradientSurfaceTagEmitter_ComponentDependencies:  Reference Gradient and Gradient Surface Tag Emitter are disabled",
            "GradientSurfaceTagEmitter_ComponentDependencies:  Shape Falloff Gradient and Gradient Surface Tag Emitter are disabled",
            "GradientSurfaceTagEmitter_ComponentDependencies:  Slope Gradient and Gradient Surface Tag Emitter are disabled",
            "GradientSurfaceTagEmitter_ComponentDependencies:  Surface Mask Gradient and Gradient Surface Tag Emitter are disabled",
        ]

        hydra.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "GradientSurfaceTagEmitter_ComponentDependencies.py",
            expected_lines=expected_lines,
            unexpected_lines=unexpected_lines,
            cfg_args=cfg_args
        )

    @pytest.mark.test_case_id("C3297303")
    @pytest.mark.SUITE_periodic
    def test_GradientSurfaceTagEmitter_SurfaceTagsAddRemoveSuccessfully(self, request, editor, level,
                                                                        launcher_platform):

        expected_lines = [
            "Entity has a Gradient Surface Tag Emitter component",
            "Entity has a Reference Gradient component",
            "Added SurfaceTag: container count is 1",
            "Removed SurfaceTag: container count is 0",
            "GradientSurfaceTagEmitter_SurfaceTagsAddRemoveSucessfully:  result=SUCCESS"
        ]

        hydra.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "GradientSurfaceTagEmitter_SurfaceTagsAddRemoveSuccessfully.py",
            expected_lines,
            cfg_args=[level]
        )
