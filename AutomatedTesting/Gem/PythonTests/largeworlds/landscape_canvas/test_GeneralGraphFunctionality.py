"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

"""
C2735988 - Landscape Canvas tool can be opened/closed
C13815862 - New graph can be created
C13767840 - New root entity is created when a new graph is created through Landscape Canvas
"""

import os
import pytest

# Bail on the test if ly_test_tools doesn't exist.
pytest.importorskip("ly_test_tools")
import ly_test_tools.environment.file_system as file_system
import editor_python_test_tools.hydra_test_utils as hydra

test_directory = os.path.join(os.path.dirname(__file__), "EditorScripts")


@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("level", ["tmp_level"])
@pytest.mark.usefixtures("automatic_process_killer")
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
class TestGeneralGraphFunctionality(object):
    @pytest.fixture(autouse=True)
    def setup_teardown(self, request, workspace, project, level):
        def teardown():
            file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)
            file_system.delete([os.path.join(workspace.paths.engine_root(), project, "slices", "TestSlice.slice")], True, True)

        request.addfinalizer(teardown)

        file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)
        file_system.delete([os.path.join(workspace.paths.engine_root(), project, "slices", "TestSlice.slice")], True, True)

    @pytest.mark.test_case_id("C2735988", "C13815862", "C13767840")
    @pytest.mark.SUITE_periodic
    def test_LandscapeCanvas_NewGraph_CreatedSuccessfully(self, request, editor, level, launcher_platform):
        cfg_args = [level]

        expected_lines = [
            "Landscape Canvas pane is open",
            "New graph created",
            "Graph registered with Landscape Canvas",
            "Root entity has Landscape Canvas component",
            "Landscape Canvas pane is closed",
            "CreateNewGraph:  result=SUCCESS",
        ]

        hydra.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "CreateNewGraph.py",
            expected_lines,
            cfg_args=cfg_args
        )

    @pytest.mark.test_case_id("C2735990")
    @pytest.mark.SUITE_periodic
    def test_LandscapeCanvas_Component_AddedRemoved(self, request, editor, level, launcher_platform):
        cfg_args = [level]

        expected_lines = [
            "Landscape Canvas Component added to Entity",
            "Landscape Canvas Component removed from Entity",
            "LandscapeCanvasComponentAddedRemoved:  result=SUCCESS",
        ]

        hydra.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "LandscapeCanvasComponent_AddedRemoved.py",
            expected_lines,
            cfg_args=cfg_args
        )

    @pytest.mark.test_case_id("C14212352")
    @pytest.mark.SUITE_periodic
    def test_LandscapeCanvas_GraphClosed_OnLevelChange(self, request, editor, level, launcher_platform):
        cfg_args = [level]

        expected_lines = [
            "Landscape Canvas pane is open",
            "New graph created",
            "Graph registered with Landscape Canvas",
            "Graph is no longer open in Landscape Canvas",
            "GraphClosedOnLevelChange:  result=SUCCESS",
        ]

        hydra.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "GraphClosed_OnLevelChange.py",
            expected_lines,
            cfg_args=cfg_args
        )

    @pytest.mark.test_case_id("C17488412")
    @pytest.mark.SUITE_periodic
    def test_LandscapeCanvas_GraphClosed_OnEntityDelete(self, request, editor, level, launcher_platform):
        cfg_args = [level]

        expected_lines = [
            "Landscape Canvas pane is open",
            "Graph registered with Landscape Canvas",
            "The graph is no longer open after deleting the Entity",
            "GraphClosedOnEntityDelete:  result=SUCCESS",
        ]

        hydra.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "GraphClosed_OnEntityDelete.py",
            expected_lines,
            cfg_args=cfg_args
        )

    @pytest.mark.test_case_id("C15167461")
    @pytest.mark.SUITE_periodic
    def test_LandscapeCanvas_GraphClosed_TabbedGraphClosesIndependently(self, request, editor, level,
                                                                        launcher_platform):
        cfg_args = [level]

        expected_lines = [
            "Landscape Canvas pane is open",
            "New graph created",
            "2nd new graph created",
            "3rd new graph created",
            "Graphs registered with Landscape Canvas",
            "Graph 2 was successfully closed",
            "GraphClosedTabbedGraph:  result=SUCCESS",
        ]

        hydra.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "GraphClosed_TabbedGraph.py",
            expected_lines,
            cfg_args=cfg_args
        )

    @pytest.mark.test_case_id("C22602016")
    @pytest.mark.SUITE_periodic
    def test_LandscapeCanvas_SliceCreateInstantiate(self, request, editor, level, workspace, launcher_platform):
        cfg_args = [level]

        expected_lines = [
            "LandscapeCanvas_SliceCreateInstantiate:  test started",
            "landscape_canvas_entity Entity successfully created",
            "LandscapeCanvas_SliceCreateInstantiate:  Slice has been created successfully: True",
            "LandscapeCanvas_SliceCreateInstantiate:  Slice instantiated: True",
            "LandscapeCanvas_SliceCreateInstantiate:  result=SUCCESS",
        ]

        hydra.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "LandscapeCanvas_SliceCreateInstantiate.py",
            expected_lines=expected_lines,
            cfg_args=cfg_args
        )
