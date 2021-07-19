"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

#
# This is a pytest module to test the in-Editor Python API from ViewPane.h
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
class TestLegacyGameModeAutomation(object):

    def test_Legacy_GameMode(self, request, editor, level, launcher_platform):

        unexpected_lines=[]
        expected_lines = [
            "Game Mode is On",
            "Game Mode is Off"
            ]
        
        test_case_file = os.path.join(os.path.dirname(__file__), 'GameModeCommands_test_case.py')
        launch_test_case(editor, test_case_file, expected_lines, unexpected_lines)
