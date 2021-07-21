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
class TestSystemSettings(object):
    @pytest.fixture(autouse=True)
    def setup_teardown(self, request, workspace, project, level):
        def teardown():
            file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)

        request.addfinalizer(teardown)

        file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)

    @pytest.mark.test_case_id("C2646869")
    @pytest.mark.SUITE_periodic
    @pytest.mark.dynveg_misc
    def test_SystemSettings_SectorPointDensity(self, request, editor, level, launcher_platform):

        expected_lines = [
            "SystemSettings_SectorPointDensity:  test started",
            "SystemSettings_SectorPointDensity:  Vegetation instances count equal to expected value before changing sector point density: True",
            "SystemSettings_SectorPointDensity:  Vegetation instances count equal to expected value after changing sector point density: True",
            "SystemSettings_SectorPointDensity:  result=SUCCESS",
        ]

        unexpected_lines = [
            "SystemSettings_SectorPointDensity:  Vegetation instances count equal to expected value before changing sector point density: False",
            "SystemSettings_SectorPointDensity:  Vegetation instances count equal to expected value after changing sector point density: False",
        ]

        hydra.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "SystemSettings_SectorPointDensity.py",
            expected_lines,
            unexpected_lines=unexpected_lines,
            cfg_args=[level]
        )

    @pytest.mark.test_case_id("C2646870")
    @pytest.mark.SUITE_periodic
    @pytest.mark.dynveg_misc
    def test_SystemSettings_SectorSize(self, request, editor, level, launcher_platform):

        expected_lines = [
            "SystemSettings_SectorSize:  test started",
            "SystemSettings_SectorSize:  Vegetation instances count equal to expected value before changing sector size: True",
            "SystemSettings_SectorSize:  Vegetation instances count equal to expected value after changing sector size: True",
            "SystemSettings_SectorSize:  result=SUCCESS",
        ]

        unexpected_lines = [
            "SystemSettings_SectorSize:  Vegetation instances count equal to expected value before changing sector size: False",
            "SystemSettings_SectorSize:  Vegetation instances count equal to expected value after changing sector size: False",
        ]

        hydra.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "SystemSettings_SectorSize.py",
            expected_lines,
            unexpected_lines=unexpected_lines,
            cfg_args=[level]
        )
