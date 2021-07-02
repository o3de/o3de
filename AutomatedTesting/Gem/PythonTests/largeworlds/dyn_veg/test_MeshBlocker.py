"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


import logging
import os
import pytest
# Bail on the test if ly_test_tools doesn't exist.
pytest.importorskip('ly_test_tools')

import editor_python_test_tools.hydra_test_utils as hydra
import ly_test_tools.environment.file_system as file_system

test_directory = os.path.join(os.path.dirname(__file__), 'EditorScripts')
logger = logging.getLogger(__name__)


@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("level", ["tmp_level"])
@pytest.mark.usefixtures("automatic_process_killer")
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
class TestMeshBlocker(object):
    @pytest.fixture(autouse=True)
    def setup_teardown(self, request, workspace, editor, project, level):
        pass

        def teardown():
            # delete temp level
            file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)

        # Setup - add the teardown finalizer
        request.addfinalizer(teardown)
        # Make sure the temp level doesn't already exist
        file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)

    """
    C3980834: A simple Vegetation Blocker Mesh can be created
    """
    @pytest.mark.test_case_id("C3980834")
    @pytest.mark.SUITE_periodic
    @pytest.mark.dynveg_area
    @pytest.mark.xfail  # LYN-3273
    def test_MeshBlocker_InstancesBlockedByMesh(self, request, editor, level, launcher_platform):
        expected_lines = [
            "'Instance Spawner' created",
            "'Surface Entity' created",
            "'Blocker Entity' created",
            "instance count validation: True (found=160, expected=160)",
            "MeshBlocker_InstancesBlockedByMesh:  result=SUCCESS"
        ]

        hydra.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "MeshBlocker_InstancesBlockedByMesh.py",
            expected_lines,
            cfg_args=[level]
        )

    """
    C4766030: Mesh Height Percent Min/Max values can be set to fine tune the blocked area
    """
    @pytest.mark.test_case_id("C4766030")
    @pytest.mark.SUITE_periodic
    @pytest.mark.dynveg_area
    @pytest.mark.xfail  # LYN-3273
    def test_MeshBlocker_InstancesBlockedByMeshHeightTuning(self, request, editor, level, launcher_platform):
        expected_lines = [
            "'Instance Spawner' created",
            "'Surface Entity' created",
            "'Blocker Entity' created",
            "Blocker Entity Configuration|Mesh Height Percent Max: SUCCESS",
            "instance count validation: True (found=127, expected=127)",
            "MeshBlocker_InstancesBlockedByMeshHeightTuning:  result=SUCCESS",
        ]

        hydra.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "MeshBlocker_InstancesBlockedByMeshHeightTuning.py",
            expected_lines,
            cfg_args=[level]
        )
