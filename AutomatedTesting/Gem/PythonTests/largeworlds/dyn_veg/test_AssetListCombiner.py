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


@pytest.mark.parametrize('project', ['AutomatedTesting'])
@pytest.mark.parametrize('level', ['tmp_level'])
@pytest.mark.usefixtures("automatic_process_killer")
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
class TestAssetListCombiner(object):

    @pytest.fixture(autouse=True)
    def setup_teardown(self, request, workspace, project, level):
        def teardown():
            file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)
        request.addfinalizer(teardown)

        file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)

    @pytest.mark.test_case_id("C4762374", "C4762373")
    @pytest.mark.SUITE_periodic
    @pytest.mark.dynveg_misc
    def test_AssetListCombiner_CombinedDescriptorsExpressInConfiguredArea(self, request, editor, level,
                                                                          launcher_platform):

        expected_lines = [
            "'Asset List 1' created",
            "'Asset List 2' created",
            "'Asset List 3' created",
            "'Surface Entity' created",
            "'Spawner Entity' created",
            "Spawner Entity Configuration|Descriptor Providers: SUCCESS",
            "Spawner Entity Configuration|Gradient|Gradient Entity Id: SUCCESS",
            "instance count validation: True (found=200, expected=200.0)",
            "Spawner Entity Configuration|Descriptor Providers|[1]: SUCCESS",
            "instance count validation: True (found=400, expected=400)",
            "instance count validation: True (found=0, expected=0)",
            "AssetListCombiner_CombinedDescriptors:  result=SUCCESS"
        ]

        hydra.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "AssetListCombiner_CombinedDescriptorsExpressInConfiguredArea.py",
            expected_lines=expected_lines,
            cfg_args=[level]
        )
