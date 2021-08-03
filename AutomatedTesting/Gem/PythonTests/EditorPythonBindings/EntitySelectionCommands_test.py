"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

#
# This is a pytest module to test the in-Editor Python API for Entity Selection
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
class TestEntitySelectionAutomation(object):

    def test_EntitySelection(self, request, editor, level, launcher_platform):

        unexpected_lines=[]
        expected_lines = [
            "SUCCESS: All Test Entities Selected",
            "SUCCESS: One Test Entity Marked as DeSelected",
            "SUCCESS: One New Test Entity Marked as Selected",
            "SUCCESS: Half of Test Entity Marked as DeSelected",
            "SUCCESS: All Test Entities Marked as Selected",
            "SUCCESS: Clear All Test Entities Selected",
            ]
        
        test_case_file = os.path.join(os.path.dirname(__file__), 'EntitySelectionCommands_test_case.py')
        launch_test_case(editor, test_case_file, expected_lines, unexpected_lines)
