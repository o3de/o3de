"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

#
# This is a pytest module to test the in-Editor Python API for Entity CRUD
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
class TestEntityCommandsAutomation(object):

    def test_EntityCommands(self, request, editor, level, launcher_platform):

        unexpected_lines=[]
        expected_lines = [
            "New entity with no parent created",
            "New root entity matches entity received from notification",
            "New entity with valid parent created",
            "First child entity matches entity received from notification",
            "Another new entity with valid parent created",
            "Second child entity matches entity received from notification",
            "Deleted all entities we created",
            "GetName and SetName work",
            "GetParent works",
            "TestName entity found",
            "TestChild entity found",
            "Test* 3 entities found"
            ]
        
        test_case_file = os.path.join(os.path.dirname(__file__), 'EntityCommands_test_case.py')
        launch_test_case(editor, test_case_file, expected_lines, unexpected_lines)
