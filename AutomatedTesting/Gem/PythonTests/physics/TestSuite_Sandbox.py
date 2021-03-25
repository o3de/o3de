"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

"""

# This suite consists of all test cases that are passing and have been verified.

import pytest
import os
import sys

from .FileManagement import FileManagement as fm
from ly_test_tools import LAUNCHERS

sys.path.append(os.path.dirname(os.path.abspath(__file__)) + '/../automatedtesting_shared')

from base import TestAutomationBase


revert_physics_config = fm.file_revert_list(['physxdebugconfiguration.setreg', 'physxdefaultsceneconfiguration.setreg', 'physxsystemconfiguration.setreg'], 'AutomatedTesting/Registry')


@pytest.mark.SUITE_sandbox
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomation(TestAutomationBase):

    ## Seems to be flaky, need to investigate
    def test_C19536274_GetCollisionName_PrintsName(self, request, workspace, editor, launcher_platform):
        from . import C19536274_GetCollisionName_PrintsName as test_module
        # Fixme: expected_lines=["Layer Name: Right"]
        self._run_test(request, workspace, editor, test_module)

    ## Seems to be flaky, need to investigate
    def test_C19536277_GetCollisionName_PrintsNothing(self, request, workspace, editor, launcher_platform):
        from . import C19536277_GetCollisionName_PrintsNothing as test_module
        # All groups present in the PhysX Collider that could show up in test
        # Fixme: collision_groups = ["All", "None", "All_NoTouchBend", "All_3", "None_1", "All_NoTouchBend_1", "All_2", "None_1_1", "All_NoTouchBend_1_1", "All_1", "None_1_1_1", "All_NoTouchBend_1_1_1", "All_4", "None_1_1_1_1", "All_NoTouchBend_1_1_1_1", "GroupLeft", "GroupRight"]
        # Fixme: for group in collision_groups:
        # Fixme:    unexpected_lines.append(f"GroupName: {group}")
        # Fixme: expected_lines=["GroupName: "]
        self._run_test(request, workspace, editor, test_module)