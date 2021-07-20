"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

"""
C13767843 - All Shape nodes can be added to a graph 
C17412059 - All Shape nodes can be removed from a graph
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
class TestShapeNodes(object):

    @pytest.fixture(autouse=True)
    def setup_teardown(self, request, workspace, project, level):
        def teardown():
            file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)
        request.addfinalizer(teardown)

        file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)

    @pytest.mark.test_case_id('C13767843')
    @pytest.mark.SUITE_periodic
    def test_LandscapeCanvas_ShapeNodes_EntityCreatedOnNodeAdd(self, request, editor, level, launcher_platform):
        cfg_args = [level]

        expected_lines = [
            "Landscape Canvas pane is open",
            "New graph created",
            "Graph registered with Landscape Canvas",
            "BoxShapeNode created new Entity with Box Shape Component",
            "CapsuleShapeNode created new Entity with Capsule Shape Component",
            "CompoundShapeNode created new Entity with Compound Shape Component",
            "CylinderShapeNode created new Entity with Cylinder Shape Component",
            "PolygonPrismShapeNode created new Entity with Polygon Prism Shape Component",
            "SphereShapeNode created new Entity with Sphere Shape Component",
            "TubeShapeNode created new Entity with Tube Shape Component",
            "DiskShapeNode created new Entity with Disk Shape Component",
            "ShapeNodeEntityCreate:  result=SUCCESS"
        ]

        hydra.launch_and_validate_results(request, test_directory, editor, 'ShapeNodes_EntityCreatedOnNodeAdd.py',
                                          expected_lines, cfg_args=cfg_args)

    @pytest.mark.test_case_id('C17412059')
    @pytest.mark.SUITE_periodic
    def test_LandscapeCanvas_ShapeNodes_EntityRemovedOnNodeDelete(self, request, editor, level, launcher_platform):
        cfg_args = [level]

        expected_lines = [
            "Landscape Canvas pane is open",
            "New graph created",
            "Graph registered with Landscape Canvas",
            "BoxShapeNode corresponding Entity was deleted when node is removed",
            "CapsuleShapeNode corresponding Entity was deleted when node is removed",
            "CompoundShapeNode corresponding Entity was deleted when node is removed",
            "CylinderShapeNode corresponding Entity was deleted when node is removed",
            "PolygonPrismShapeNode corresponding Entity was deleted when node is removed",
            "SphereShapeNode corresponding Entity was deleted when node is removed",
            "TubeShapeNode corresponding Entity was deleted when node is removed",
            "DiskShapeNode corresponding Entity was deleted when node is removed",
            "ShapeNodeEntityDelete:  result=SUCCESS"
        ]

        hydra.launch_and_validate_results(request, test_directory, editor, 'ShapeNodes_EntityRemovedOnNodeDelete.py',
                                          expected_lines, cfg_args=cfg_args)
