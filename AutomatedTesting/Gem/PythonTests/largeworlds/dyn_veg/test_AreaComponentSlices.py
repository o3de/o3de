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
class TestAreaComponents(object):
    @pytest.fixture(autouse=True)
    def setup_teardown(self, request, workspace, project, level):
        # Cleanup our temp level
        file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)

        # Cleanup the test slices
        file_system.delete([os.path.join(workspace.paths.engine_root(), project, "slices", "TestSlice_1.slice")], True, True)
        file_system.delete([os.path.join(workspace.paths.engine_root(), project, "slices", "TestSlice_2.slice")], True, True)

        def teardown():
            # Cleanup our temp level
            file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)

            # Cleanup the test slices
            file_system.delete(
                [os.path.join(workspace.paths.engine_root(), project, "slices", "TestSlice_1.slice")], True, True
            )
            file_system.delete(
                [os.path.join(workspace.paths.engine_root(), project, "slices", "TestSlice_2.slice")], True, True
            )

        request.addfinalizer(teardown)

    @pytest.mark.test_case_id("C2627900", "C2627905", "C2627904")
    @pytest.mark.SUITE_periodic
    @pytest.mark.dynveg_misc
    def test_AreaComponents_SliceCreationVisibilityToggleWorks(self, request, editor, level, workspace,
                                                              launcher_platform):
        cfg_args = [level]

        expected_lines = [
            "AreaComponentSlices_SliceCreationAndVisibilityToggle:  test started",
            "AreaComponentSlices_SliceCreationAndVisibilityToggle:  Slice has been created successfully (entity with spawner component): True",
            "AreaComponentSlices_SliceCreationAndVisibilityToggle:  Vegetation plants initially when slice is shown: True",
            "AreaComponentSlices_SliceCreationAndVisibilityToggle:  Vegetation is cleared when slice is hidden: True",
            "AreaComponentSlices_SliceCreationAndVisibilityToggle:  Slice has been created successfully (entity with blender component): True",
            "AreaComponentSlices_SliceCreationAndVisibilityToggle:  result=SUCCESS",
        ]

        hydra.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "AreaComponentSlices_SliceCreationAndVisibilityToggle.py",
            expected_lines=expected_lines,
            cfg_args=cfg_args
        )
