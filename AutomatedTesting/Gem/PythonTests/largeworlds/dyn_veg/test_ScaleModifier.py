"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

"""
C4814462: Vegetation instances have random scale between 0.1 and 1.0 applied.
"""

import os
import pytest

# Bail on the test if ly_test_tools doesn't exist.
pytest.importorskip("ly_test_tools")

import automatedtesting_shared.hydra_test_utils as hydra
import ly_test_tools.environment.file_system as file_system

test_directory = os.path.join(os.path.dirname(__file__), "EditorScripts")


@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("level", ["tmp_level"])
@pytest.mark.usefixtures("automatic_process_killer")
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
class TestScaleOverrideWorksSuccessfully(object):

    @pytest.fixture(autouse=True)
    def setup_teardown(self, request, workspace, project, level):
        def teardown():
            file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)
        request.addfinalizer(teardown)

        file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)

    @pytest.mark.test_case_id("C4814462")
    @pytest.mark.SUITE_periodic
    def test_ScaleModifierOverrides_InstancesProperlyScale(self, request, editor, level, launcher_platform):

        expected_lines = [
            "'Spawner Entity' created",
            "'Surface Entity' created",
            "Entity has a Vegetation Scale Modifier component",
            "'Gradient Entity' created",
            "Scale Min and Scale Max are set to 0.1 and 1.0 in Vegetation Asset List",
            "Entity has a Random Noise Gradient component",
            "Entity has a Gradient Transform Modifier component",
            "Entity has a Box Shape component",
            "Spawner Entity Configuration|Gradient|Gradient Entity Id: SUCCESS",
            "ScaleModifierOverrides_InstancesProperlyScale:  result=SUCCESS"
        ]

        unexpected_lines = ["Scale Min and Scale Max are not set to 0.1 and 1.0 in Vegetation Asset List"]

        hydra.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "ScaleModifierOverrides_InstancesProperlyScale.py",
            expected_lines,
            unexpected_lines,
            cfg_args=[level]
        )

    @pytest.mark.test_case_id("C4896937")
    @pytest.mark.SUITE_periodic
    def test_ScaleModifier_InstancesProperlyScale(self, request, editor, level, launcher_platform):

        expected_lines = [
            "'Spawner Entity' created",
            "Entity has a Vegetation Scale Modifier component",
            "'Surface Entity' created",
            "'Gradient Entity' created",
            "Spawner Entity Configuration|Gradient|Gradient Entity Id: SUCCESS",
            "Spawner Entity Configuration|Range Min: SUCCESS",
            "Spawner Entity Configuration|Range Max: SUCCESS",
            "ScaleModifier_InstancesProperlyScale:  result=SUCCESS"
        ]

        hydra.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "ScaleModifier_InstancesProperlyScale.py",
            expected_lines,
            cfg_args=[level]
        )
