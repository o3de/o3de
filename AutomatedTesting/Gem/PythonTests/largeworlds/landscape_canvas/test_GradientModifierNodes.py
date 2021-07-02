"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

"""
C13767841 - All Gradient Modifier nodes can be added to a graph
C18055051 - All Gradient Modifier nodes can be removed from a graph
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
class TestGradientModifierNodes(object):

    @pytest.fixture(autouse=True)
    def setup_teardown(self, request, workspace, project, level):
        def teardown():
            file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)
        request.addfinalizer(teardown)

        file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)

    @pytest.mark.test_case_id('C13767841')
    @pytest.mark.SUITE_periodic
    def test_LandscapeCanvas_GradientModifierNodes_EntityCreatedOnNodeAdd(self, request, editor, level,
                                                                          launcher_platform):
        """
        Verifies all Gradient Modifier nodes can be successfully added to a Landscape Canvas graph, and the proper
        entity creation occurs.
        """
        cfg_args = [level]

        expected_lines = [
            "Landscape Canvas pane is open",
            "New graph created",
            "Graph registered with Landscape Canvas",
            "DitherGradientModifierNode created new Entity with Dither Gradient Modifier Component",
            "GradientMixerNode created new Entity with Gradient Mixer Component",
            "InvertGradientModifierNode created new Entity with Invert Gradient Modifier Component",
            "LevelsGradientModifierNode created new Entity with Levels Gradient Modifier Component",
            "PosterizeGradientModifierNode created new Entity with Posterize Gradient Modifier Component",
            "SmoothStepGradientModifierNode created new Entity with Smooth-Step Gradient Modifier Component",
            "ThresholdGradientModifierNode created new Entity with Threshold Gradient Modifier Component",
            "GradientModifierNodeEntityCreate:  result=SUCCESS"
        ]

        hydra.launch_and_validate_results(request, test_directory, editor,
                                          'GradientModifierNodes_EntityCreatedOnNodeAdd.py',
                                          expected_lines, cfg_args=cfg_args)

    @pytest.mark.test_case_id('C18055051')
    @pytest.mark.SUITE_periodic
    def test_LandscapeCanvas_GradientModifierNodes_EntityRemovedOnNodeDelete(self, request, editor, level,
                                                                             launcher_platform):
        """
        Verifies all Gradient Modifier nodes can be successfully removed from a Landscape Canvas graph, and the proper
        entity cleanup occurs.
        """
        cfg_args = [level]

        expected_lines = [
            "Landscape Canvas pane is open",
            "New graph created",
            "Graph registered with Landscape Canvas",
            "DitherGradientModifierNode corresponding Entity was deleted when node is removed",
            "GradientMixerNode corresponding Entity was deleted when node is removed",
            "InvertGradientModifierNode corresponding Entity was deleted when node is removed",
            "LevelsGradientModifierNode corresponding Entity was deleted when node is removed",
            "PosterizeGradientModifierNode corresponding Entity was deleted when node is removed",
            "SmoothStepGradientModifierNode corresponding Entity was deleted when node is removed",
            "ThresholdGradientModifierNode corresponding Entity was deleted when node is removed",
            "GradientModifierNodeEntityDelete:  result=SUCCESS"
        ]

        hydra.launch_and_validate_results(request, test_directory, editor,
                                          'GradientModifierNodes_EntityRemovedOnNodeDelete.py',
                                          expected_lines, cfg_args=cfg_args)
