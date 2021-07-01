"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

"""
C13815920 - Appropriate component dependencies are automatically added to node entities
C13767842 - All Gradient nodes can be added to a graph
C17461363 - All Gradient nodes can be removed from a graph
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
class TestGradientNodes(object):

    @pytest.fixture(autouse=True)
    def setup_teardown(self, request, workspace, project, level):
        def teardown():
            file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)
        request.addfinalizer(teardown)

        file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)

    @pytest.mark.test_case_id('C13815920')
    @pytest.mark.SUITE_periodic
    def test_LandscapeCanvas_GradientNodes_DependentComponentsAdded(self, request, editor, level, launcher_platform):
        cfg_args = [level]

        expected_lines = [
            "Landscape Canvas pane is open",
            "New graph created",
            "Graph registered with Landscape Canvas",
            "FastNoiseGradientNode created new Entity with all required components",
            "ImageGradientNode created new Entity with all required components",
            "PerlinNoiseGradientNode created new Entity with all required components",
            "RandomNoiseGradientNode created new Entity with all required components"
        ]

        hydra.launch_and_validate_results(request, test_directory, editor, 'GradientNodes_DependentComponentsAdded.py',
                                          expected_lines, cfg_args=cfg_args)

    @pytest.mark.test_case_id('C13767842')
    @pytest.mark.SUITE_periodic
    def test_LandscapeCanvas_GradientNodes_EntityCreatedOnNodeAdd(self, request, editor, level, launcher_platform):
        """
        Verifies all Gradient nodes can be successfully added to a Landscape Canvas graph, and the proper entity
        creation occurs.
        """
        cfg_args = [level]

        expected_lines = [
            "Landscape Canvas pane is open",
            "New graph created",
            "Graph registered with Landscape Canvas",
            "AltitudeGradientNode created new Entity with Altitude Gradient Component",
            "ConstantGradientNode created new Entity with Constant Gradient Component",
            "FastNoiseGradientNode created new Entity with FastNoise Gradient Component",
            "ImageGradientNode created new Entity with Image Gradient Component",
            "PerlinNoiseGradientNode created new Entity with Perlin Noise Gradient Component",
            "RandomNoiseGradientNode created new Entity with Random Noise Gradient Component",
            "ShapeAreaFalloffGradientNode created new Entity with Shape Falloff Gradient Component",
            "SlopeGradientNode created new Entity with Slope Gradient Component",
            "SurfaceMaskGradientNode created new Entity with Surface Mask Gradient Component"
        ]

        hydra.launch_and_validate_results(request, test_directory, editor, 'GradientNodes_EntityCreatedOnNodeAdd.py',
                                          expected_lines, cfg_args=cfg_args)

    @pytest.mark.test_case_id('C17461363')
    @pytest.mark.SUITE_periodic
    def test_LandscapeCanvas_GradientNodes_EntityRemovedOnNodeDelete(self, request, editor, level, launcher_platform):
        """
        Verifies all Gradient nodes can be successfully removed from a Landscape Canvas graph, and the proper entity
        cleanup occurs.
        """
        cfg_args = [level]

        expected_lines = [
            "Landscape Canvas pane is open",
            "New graph created",
            "Graph registered with Landscape Canvas",
            "FastNoiseGradientNode corresponding Entity was deleted when node is removed",
            "AltitudeGradientNode corresponding Entity was deleted when node is removed",
            "ConstantGradientNode corresponding Entity was deleted when node is removed",
            "RandomNoiseGradientNode corresponding Entity was deleted when node is removed",
            "ShapeAreaFalloffGradientNode corresponding Entity was deleted when node is removed",
            "SlopeGradientNode corresponding Entity was deleted when node is removed",
            "PerlinNoiseGradientNode corresponding Entity was deleted when node is removed",
            "ImageGradientNode corresponding Entity was deleted when node is removed",
            "SurfaceMaskGradientNode corresponding Entity was deleted when node is removed"
        ]

        hydra.launch_and_validate_results(request, test_directory, editor, 'GradientNodes_EntityRemovedOnNodeDelete.py',
                                          expected_lines, cfg_args=cfg_args)
