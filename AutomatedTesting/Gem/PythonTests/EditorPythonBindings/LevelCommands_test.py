"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

#
# This is a pytest module to test the in-Editor Python API from PythonEditorFuncs
#
import pytest
pytest.importorskip('ly_test_tools')

import sys
import os
sys.path.append(os.path.dirname(__file__))
from hydra_utils import launch_test_case


@pytest.mark.SUITE_sandbox
@pytest.mark.parametrize('launcher_platform', ['windows_editor'])
@pytest.mark.parametrize('project', ['AutomatedTesting'])
@pytest.mark.parametrize('level', ['Simple'])
class TestLevelAutomation(object):

    def test_LevelCommands(self, request, editor, level, launcher_platform):

        unexpected_lines=[]
        expected_lines = [
            "WaterSample level has already been created",
            "Editor with WaterSample level opened closed",
            "WaterSample level opened",
            "Level name is correct",
            "The level is in the Levels folder",
            "Game folder is correct"
            ]
        
        test_case_file = os.path.join(os.path.dirname(__file__), 'LevelCommands_test_case.py')
        launch_test_case(editor, test_case_file, expected_lines, unexpected_lines)

