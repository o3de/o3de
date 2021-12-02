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
class TestEntityCRUDCommandsAutomation(object):

    def test_EntityCRUDCommands(self, request, editor, level, launcher_platform):

        unexpected_lines=[]
        expected_lines = [
            "childEntity is parented to the Root",
            "childEntity is now parented to the parentEntity",
            "parentEntityId isn't locked",
            "parentEntityId is now locked",
            "childEntityId is now locked too",
            "parentEntityId is visible",
            "parentEntityId isn't hidden",
            "parentEntityId is now hidden",
            "childEntityId is now hidden too",
            "parentEntityId isn't set to Editor Only",
            "parentEntityId is now set to Editor Only",
            "childEntityId does not inherit Editor Only",
            "parentEntityId isn't set to Start Inactive",
            "parentEntityId is now set to Start Inactive",
            "childEntityId does not inherit Start Inactive",
            "parentEntityId isn't set to Start Active",
            "parentEntityId is now set to Start Active",
            "childEntityId should still be set to Start Active by default"
            ]
        
        test_case_file = os.path.join(os.path.dirname(__file__), 'EntityCRUDCommands_test_case.py')
        launch_test_case(editor, test_case_file, expected_lines, unexpected_lines)
