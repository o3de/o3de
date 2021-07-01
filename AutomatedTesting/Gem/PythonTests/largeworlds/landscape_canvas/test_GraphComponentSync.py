"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

"""
C4705586 - Altering connections on graph nodes appropriately updates component properties
C22715182 - Components are updated when nodes are added/removed/updated
C22602072 - Graph is updated when underlying components are added/removed
C15987206 - Gradient Mixer Layers are properly setup when constructing in a graph
C21333743 - Vegetation Layer Blenders are properly setup when constructing in a graph
"""

import os
import pytest

# Bail on the test if ly_test_tools doesn't exist.
pytest.importorskip('ly_test_tools')
import ly_test_tools.environment.file_system as file_system
import ly_test_tools._internal.pytest_plugin as internal_plugin
import editor_python_test_tools.hydra_test_utils as hydra

test_directory = os.path.join(os.path.dirname(__file__), 'EditorScripts')


@pytest.mark.parametrize('project', ['AutomatedTesting'])
@pytest.mark.parametrize('level', ['tmp_level'])
@pytest.mark.usefixtures("automatic_process_killer")
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
class TestGraphComponentSync(object):

    @pytest.fixture(autouse=True)
    def setup_teardown(self, request, workspace, project, level):
        def teardown():
            file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)
        request.addfinalizer(teardown)

        file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)

    @pytest.mark.test_case_id('C4705586')
    @pytest.mark.BAT
    @pytest.mark.SUITE_main
    def test_LandscapeCanvas_SlotConnections_UpdateComponentReferences(self, request, editor, level, launcher_platform):

        # Skip test if running against Debug build
        if "debug" in internal_plugin.build_directory:
            pytest.skip("Does not execute against debug builds.")

        cfg_args = [level]

        expected_lines = [
            "Landscape Canvas pane is open",
            "New graph created",
            "Graph registered with Landscape Canvas",
            "Random Noise Gradient component Preview Entity property set to Box Shape EntityId",
            "Dither Gradient Modifier component Inbound Gradient property set to Random Noise Gradient EntityId",
            "Gradient Mixer component Inbound Gradient extendable property set to Dither Gradient Modifier EntityId",
            "SlotConnectionsUpdateComponents:  result=SUCCESS"
        ]

        hydra.launch_and_validate_results(request, test_directory, editor,
                                          'SlotConnections_UpdateComponentReferences.py', expected_lines,
                                          cfg_args=cfg_args)

    @pytest.mark.test_case_id('C22715182')
    @pytest.mark.SUITE_periodic
    def test_LandscapeCanvas_GraphUpdates_UpdateComponents(self, request, editor, level, launcher_platform):
        cfg_args = [level]

        expected_lines = [
            'Rotation Modifier component was removed from entity',
            'BushSpawner entity was deleted',
            'Gradient Entity Id reference was properly updated',
            'GraphUpdatesUpdateComponents:  result=SUCCESS'
        ]

        unexpected_lines = [
            'Rotation Modifier component is still present on entity',
            'Failed to delete BushSpawner entity',
            'Gradient Entity Id was not updated properly'
        ]

        hydra.launch_and_validate_results(request, test_directory, editor, 'GraphUpdates_UpdateComponents.py',
                                          expected_lines, unexpected_lines=unexpected_lines,
                                          cfg_args=cfg_args)

    @pytest.mark.test_case_id('C22602072')
    @pytest.mark.SUITE_periodic
    def test_LandscapeCanvas_ComponentUpdates_UpdateGraph(self, request, editor, level, launcher_platform):
        cfg_args = [level]

        expected_lines = [
            "LandscapeCanvas entity found",
            "BushSpawner entity found",
            "Vegetation Distribution Filter on BushSpawner entity found",
            "Graph opened",
            "Distribution Filter node found on graph",
            "Vegetation Altitude Filter on BushSpawner entity found",
            "Altitude Filter node found on graph",
            "Vegetation Distribution Filter removed from BushSpawner entity",
            "Distribution Filter node was removed from the graph",
            "New entity successfully added as a child of the BushSpawner entity",
            "Box Shape on Box entity found",
            "Box Shape node found on graph",
            'ComponentUpdatesUpdateGraph:  result=SUCCESS'
        ]

        unexpected_lines = [
            "Distribution Filter node not found on graph",
            "Distribution Filter node is still present on the graph",
            "Altitude Filter node not found on graph",
            "New entity added with an unexpected parent",
            "Box Shape node not found on graph"
        ]

        hydra.launch_and_validate_results(request, test_directory, editor, 'ComponentUpdates_UpdateGraph.py',
                                          expected_lines, unexpected_lines=unexpected_lines, cfg_args=cfg_args)

    @pytest.mark.test_case_id('C15987206')
    @pytest.mark.SUITE_main
    def test_LandscapeCanvas_GradientMixer_NodeConstruction(self, request, editor, level, launcher_platform):
        """
        Verifies a Gradient Mixer can be setup in Landscape Canvas and all references are property set.
        """

        # Skip test if running against Debug build
        if "debug" in internal_plugin.build_directory:
            pytest.skip("Does not execute against debug builds.")

        cfg_args = [level]

        expected_lines = [
            'Landscape Canvas pane is open',
            'New graph created',
            'Graph registered with Landscape Canvas',
            'Perlin Noise Gradient component Preview Entity property set to Box Shape EntityId',
            'Gradient Mixer component Inbound Gradient extendable property set to Perlin Noise Gradient EntityId',
            'Gradient Mixer component Inbound Gradient extendable property set to FastNoise Gradient EntityId',
            'Configuration|Layers|[0]|Operation set to 0',
            'Configuration|Layers|[1]|Operation set to 6',
            'GradientMixerNodeConstruction:  result=SUCCESS'
        ]

        hydra.launch_and_validate_results(request, test_directory, editor, 'GradientMixer_NodeConstruction.py',
                                          expected_lines, cfg_args=cfg_args)

    @pytest.mark.test_case_id('C21333743')
    @pytest.mark.SUITE_periodic
    def test_LandscapeCanvas_LayerBlender_NodeConstruction(self, request, editor, level, launcher_platform):
        """
        Verifies a Layer Blender can be setup in Landscape Canvas and all references are property set.
        """
        cfg_args = [level]

        expected_lines = [
            'Landscape Canvas pane is open',
            'New graph created',
            'Graph registered with Landscape Canvas',
            'Vegetation Layer Blender component Vegetation Areas[0] property set to Vegetation Layer Spawner EntityId',
            'Vegetation Layer Blender component Vegetation Areas[1] property set to Vegetation Layer Blocker EntityId',
            'LayerBlenderNodeConstruction:  result=SUCCESS'
        ]

        hydra.launch_and_validate_results(request, test_directory, editor, 'LayerBlender_NodeConstruction.py',
                                          expected_lines, cfg_args=cfg_args)
