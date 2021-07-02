"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

"""
C29278563 - Disabled nodes can be successfully duplicated
C30813586 - Editor remains stable after Undoing deletion of a node on a slice entity
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
class TestEditFunctionality(object):

    @pytest.fixture(autouse=True)
    def setup_teardown(self, request, workspace, project, level):
        def teardown():
            file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)
        request.addfinalizer(teardown)

        file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)

    @pytest.mark.test_case_id('C29278563')
    @pytest.mark.SUITE_periodic
    def test_LandscapeCanvas_DuplicateDisabledNodes(self, request, editor, level, launcher_platform):
        cfg_args = [level]

        expected_lines = [
            "Landscape Canvas pane is open",
            "New graph created",
            "SpawnerAreaNode duplicated with disabled component",
            "SpawnerAreaNode duplicated with deleted component",
            "MeshBlockerAreaNode duplicated with disabled component",
            "MeshBlockerAreaNode duplicated with deleted component",
            "BlockerAreaNode duplicated with disabled component",
            "BlockerAreaNode duplicated with deleted component",
            "FastNoiseGradientNode duplicated with disabled component",
            "FastNoiseGradientNode duplicated with deleted component",
            "ImageGradientNode duplicated with disabled component",
            "ImageGradientNode duplicated with deleted component",
            "PerlinNoiseGradientNode duplicated with disabled component",
            "PerlinNoiseGradientNode duplicated with deleted component",
            "RandomNoiseGradientNode duplicated with disabled component",
            "RandomNoiseGradientNode duplicated with deleted component",
            "DisabledNodeDuplication:  result=SUCCESS"
        ]

        hydra.launch_and_validate_results(request, test_directory, editor, 'Edit_DisabledNodeDuplication.py',
                                          expected_lines, cfg_args=cfg_args)

    @pytest.mark.test_case_id('C30813586')
    @pytest.mark.SUITE_periodic
    def test_LandscapeCanvas_UndoNodeDelete_SliceEntity(self, request, editor, level, launcher_platform):
        cfg_args = [level]

        expected_lines = [
            "Vegetation Layer Spawner node found on graph",
            "Vegetation Layer Spawner node was removed",
            "Editor is still responsive",
            "UndoNodeDeleteSlice:  result=SUCCESS"
        ]

        unexpected_lines = [
            "Vegetation Layer Spawner node not found",
            "Vegetation Layer Spawner node was not removed"
        ]

        hydra.launch_and_validate_results(request, test_directory, editor, 'Edit_UndoNodeDelete_SliceEntity.py',
                                          expected_lines, unexpected_lines=unexpected_lines, cfg_args=cfg_args)
