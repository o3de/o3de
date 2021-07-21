"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

"""
C13815919 - Appropriate component dependencies are automatically added to node entities
C13767844 - All Vegetation Area nodes can be added to a graph
C17605868 - All Vegetation Area nodes can be removed from a graph
C13815873 - All Filters/Modifiers/Selectors can be added to/removed from a Layer node
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
class TestAreaNodes(object):

    @pytest.fixture(autouse=True)
    def setup_teardown(self, request, workspace, project, level):
        def teardown():
            file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)

        request.addfinalizer(teardown)

        file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)

    @pytest.mark.test_case_id('C13815919')
    @pytest.mark.SUITE_periodic
    def test_LandscapeCanvas_AreaNodes_DependentComponentsAdded(self, request, editor, level, launcher_platform):
        cfg_args = [level]

        expected_lines = [
            "Landscape Canvas pane is open",
            "New graph created",
            "Graph registered with Landscape Canvas",
            "SpawnerAreaNode created new Entity with all required components",
            "MeshBlockerAreaNode created new Entity with all required components",
            "BlockerAreaNode created new Entity with all required components",
            "AreaNodeComponentDependency:  result=SUCCESS"
        ]

        hydra.launch_and_validate_results(request, test_directory, editor, 'AreaNodes_DependentComponentsAdded.py',
                                          expected_lines, cfg_args=cfg_args)

    @pytest.mark.test_case_id('C13767844')
    @pytest.mark.SUITE_periodic
    def test_LandscapeCanvas_AreaNodes_EntityCreatedOnNodeAdd(self, request, editor, level, launcher_platform):
        """
        Verifies all Area nodes can be successfully added to a Landscape Canvas graph, and the proper entity
        creation occurs.
        """
        cfg_args = [level]

        expected_lines = [
            "Landscape Canvas pane is open",
            "New graph created",
            "Graph registered with Landscape Canvas",
            "AreaBlenderNode created new Entity with Vegetation Layer Blender Component",
            "BlockerAreaNode created new Entity with Vegetation Layer Blocker Component",
            "MeshBlockerAreaNode created new Entity with Vegetation Layer Blocker (Mesh) Component",
            "SpawnerAreaNode created new Entity with Vegetation Layer Spawner Component",
            "AreaNodeEntityCreate:  result=SUCCESS"
        ]

        hydra.launch_and_validate_results(request, test_directory, editor, 'AreaNodes_EntityCreatedOnNodeAdd.py',
                                          expected_lines, cfg_args=cfg_args)

    @pytest.mark.test_case_id('C17605868')
    @pytest.mark.SUITE_periodic
    def test_LandscapeCanvas_AreaNodes_EntityRemovedOnNodeDelete(self, request, editor, level, launcher_platform):
        """
        Verifies all Area nodes can be successfully removed from a Landscape Canvas graph, and the proper entity
        cleanup occurs.
        """
        cfg_args = [level]

        expected_lines = [
            "Landscape Canvas pane is open",
            "New graph created",
            "Graph registered with Landscape Canvas",
            "AreaBlenderNode corresponding Entity was deleted when node is removed",
            "MeshBlockerAreaNode corresponding Entity was deleted when node is removed",
            "SpawnerAreaNode corresponding Entity was deleted when node is removed",
            "BlockerAreaNode corresponding Entity was deleted when node is removed",
            "AreaNodeEntityDelete:  result=SUCCESS"
        ]

        hydra.launch_and_validate_results(request, test_directory, editor,
                                          'AreaNodes_EntityRemovedOnNodeDelete.py', expected_lines, cfg_args=cfg_args)

    @pytest.mark.test_case_id('C13815873')
    @pytest.mark.SUITE_periodic
    def test_LandscapeCanvas_LayerExtenderNodes_ComponentEntitySync(self, request, editor, level, launcher_platform):
        """
        Verifies all Area Extender nodes can be successfully added to and removed from a Landscape Canvas graph, and the
        proper entity creation/cleanup occurs.
        """
        cfg_args = [level]

        expected_lines = [
            "Landscape Canvas pane is open",
            "New graph created",
            "Graph registered with Landscape Canvas",
            "AreaBlenderNode successfully added and removed all filters/modifiers/selectors",
            "SpawnerAreaNode successfully added and removed all filters/modifiers/selectors",
            "LayerExtenderNodeComponentEntitySync:  result=SUCCESS"
        ]

        hydra.launch_and_validate_results(request, test_directory, editor,
                                          'LayerExtenderNodes_ComponentEntitySync.py', expected_lines,
                                          cfg_args=cfg_args)
