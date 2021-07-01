"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
import pytest
import logging

# Bail on the test if ly_test_tools doesn't exist.
pytest.importorskip('ly_test_tools')
import ly_test_tools.environment.file_system as file_system
import editor_python_test_tools.hydra_test_utils as hydra

logger = logging.getLogger(__name__)
test_directory = os.path.join(os.path.dirname(__file__), 'EditorScripts')


@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("level", ["tmp_level"])
@pytest.mark.usefixtures("automatic_process_killer")
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
class TestDistanceBetweenFilter(object):

    @pytest.fixture(autouse=True)
    def setup_teardown(self, request, workspace, project, level):
        def teardown():
            file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)
        request.addfinalizer(teardown)

        file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)

    @pytest.mark.test_case_id("C4851066")
    @pytest.mark.SUITE_periodic
    @pytest.mark.dynveg_filter
    def test_DistanceBetweenFilter_InstancesPlantAtSpecifiedRadius(self, request, editor, level, launcher_platform):

        expected_lines = [
            "Configuration|Radius Min set to 1.0",
            "Configuration|Radius Min set to 2.0",
            "Configuration|Radius Min set to 16.0",
            "DistanceBetweenFilterComponent:  result=SUCCESS"
        ]

        hydra.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "DistanceBetweenFilter_InstancesPlantAtSpecifiedRadius.py",
            expected_lines,
            cfg_args=[level]
        )

    @pytest.mark.test_case_id("C4814458")
    @pytest.mark.SUITE_periodic
    @pytest.mark.dynveg_filter
    def test_DistanceBetweenFilterOverrides_InstancesPlantAtSpecifiedRadius(self, request, editor, level,
                                                                            launcher_platform):

        expected_lines = [
            "Configuration|Embedded Assets|[0]|Distance Between Filter (Radius)|Radius Min set to 1.0",
            "Configuration|Embedded Assets|[0]|Distance Between Filter (Radius)|Radius Min set to 2.0",
            "Configuration|Embedded Assets|[0]|Distance Between Filter (Radius)|Radius Min set to 16.0",
            "DistanceBetweenFilterComponentOverrides:  result=SUCCESS"
        ]

        hydra.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "DistanceBetweenFilterOverrides_InstancesPlantAtSpecifiedRadius.py",
            expected_lines,
            cfg_args=[level]
        )
