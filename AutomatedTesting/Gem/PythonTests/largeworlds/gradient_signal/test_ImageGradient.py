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
class TestImageGradientRequiresShape(object):
    @pytest.fixture(autouse=True)
    def setup_teardown(self, request, workspace, project, level):
        def teardown():
            file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)

        request.addfinalizer(teardown)

    @pytest.mark.test_case_id('C2707570')
    @pytest.mark.SUITE_periodic
    def test_ImageGradient_RequiresShape(self, request, editor, level, launcher_platform):
        cfg_args = [level]
        expected_lines = [
            "Image Gradient component was added to entity, but the component is disabled",
            "Gradient Transform Modifier component was added to entity, but the component is disabled",
            "Image Gradient component is not active without a Shape component on the Entity",
            "Box Shape component was added to entity",
            "Image Gradient component is active now that the Entity has a Shape",
            "ImageGradientRequiresShape:  result=SUCCESS"
        ]

        hydra.launch_and_validate_results(request, test_directory, editor,
                                          'ImageGradient_RequiresShape.py',
                                          expected_lines=expected_lines, cfg_args=cfg_args)

    @pytest.mark.test_case_id("C3829430")
    @pytest.mark.SUITE_periodic
    def test_ImageGradient_ProcessedImageAssignedSuccessfully(self, request, editor, level, launcher_platform):

        expected_lines = [
            "Image Gradient Entity created",
            "Entity has a Image Gradient component",
            "Entity has a Gradient Transform Modifier component",
            "Entity has a Box Shape component",
            "image_grad_test_gsi.png was found in the workspace",
            "Entity Configuration|Image Asset: SUCCESS",
            "ImageGradient_ProcessedImageAssignedSucessfully:  result=SUCCESS",
        ]

        hydra.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "ImageGradient_ProcessedImageAssignedSuccessfully.py",
            expected_lines,
            cfg_args=[level]
        )
