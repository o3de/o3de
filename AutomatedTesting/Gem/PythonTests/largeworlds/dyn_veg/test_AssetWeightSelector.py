"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

"""
C6269654: Vegetation areas using weight selectors properly distribute instances according to Sort By Weight setting
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
class TestAssetWeightSelector(object):

    @pytest.fixture(autouse=True)
    def setup_teardown(self, request, workspace, project, level):
        def teardown():
            file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)
        request.addfinalizer(teardown)

        file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)

    @pytest.mark.test_case_id("C6269654", "C4762368")
    @pytest.mark.SUITE_sandbox
    @pytest.mark.dynveg_filter
    def test_AssetWeightSelector_InstancesExpressBasedOnWeight(self, request, editor, level, launcher_platform):

        expected_lines = [
            "'Instance Spawner' created",
            "Instance Spawner Configuration|Embedded Assets|[1]|Instance|Slice Asset: SUCCESS",
            "'Planting Surface' created",
            "Configuration|Embedded Assets|[0]|Weight set to 50.0",
            "Instance Spawner Configuration|Allow Empty Assets: SUCCESS",
            "AssetWeightSelector_SortByWeight:  result=SUCCESS",
        ]

        hydra.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "AssetWeightSelector_InstancesExpressBasedOnWeight.py",
            expected_lines,
            cfg_args=[level]
        )
