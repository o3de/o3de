"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

import logging
import os
import pytest

# Bail on the test if ly_test_tools doesn't exist.
pytest.importorskip("ly_test_tools")

import automatedtesting_shared.hydra_test_utils as hydra
import ly_test_tools.environment.file_system as file_system

logger = logging.getLogger(__name__)

test_directory = os.path.join(os.path.dirname(__file__), "EditorScripts")


@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("level", ["tmp_level"])
@pytest.mark.usefixtures("automatic_process_killer")
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
class TestRotationModifier(object):
    @pytest.fixture(autouse=True)
    def setup_teardown(self, request, workspace, project, level):
        def teardown():
            # delete temp level
            file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)

        # Setup - add the teardown finalizer
        request.addfinalizer(teardown)

        file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)

    @pytest.mark.test_case_id("C4896922")
    @pytest.mark.SUITE_periodic
    def test_RotationModifier_InstancesRotateWithinRange(self, request, editor, level, launcher_platform) -> None:
        """
        Launches editor and run test script to test that rotation modifier works for all axis.
        Manual test case: C4896922
        """

        expected_lines = [
            "'Spawner Entity' created",
            "'Surface Entity' created",
            "'Gradient Entity' created",
            "Entity has a Vegetation Asset List component",
            "Entity has a Vegetation Layer Spawner component",
            "Entity has a Vegetation Rotation Modifier component",
            "Entity has a Box Shape component",
            "Entity has a Constant Gradient component",
            "RotationModifier_InstancesRotateWithinRange:  result=SUCCESS"
        ]

        hydra.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "RotationModifier_InstancesRotateWithinRange.py",
            expected_lines,
            cfg_args=[level]
        )

    @pytest.mark.test_case_id("C4814460")
    @pytest.mark.SUITE_periodic
    def test_RotationModifierOverrides_InstancesRotateWithinRange(self, request, editor, level, launcher_platform) -> None:

        expected_lines = [
            "'Spawner Entity' created",
            "'Surface Entity' created",
            "'Gradient Entity' created",
            "Entity has a Vegetation Layer Spawner component",
            "Entity has a Vegetation Asset List component",
            "Spawner Entity Box Shape|Box Configuration|Dimensions: SUCCESS",
            "Entity has a Vegetation Rotation Modifier component",
            "Spawner Entity Configuration|Embedded Assets|[0]|Rotation Modifier|Override Enabled: SUCCESS",
            "Spawner Entity Configuration|Allow Per-Item Overrides: SUCCESS",
            "Entity has a Constant Gradient component",
            "Entity has a Box Shape component",
            "Spawner Entity Configuration|Rotation Z|Gradient|Gradient Entity Id: SUCCESS",
            "RotationModifierOverrides_InstancesRotateWithinRange:  result=SUCCESS"
        ]

        hydra.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "RotationModifierOverrides_InstancesRotateWithinRange.py",
            expected_lines,
            cfg_args=[level]
        )
