"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

"""

# This suite consists of all test cases that are passing and have been verified.

import pytest
import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)) + '/../Physics')

from utils.FileManagement import FileManagement as fm
from ly_test_tools import LAUNCHERS

from base import TestAutomationBase

@pytest.mark.SUITE_main
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomation(TestAutomationBase):

    def test_AxisAlignedBoxShape_ConfigurationWorks(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import TerrainPhysicsCollider_ChangesSizeWithAxisAlignedBoxShapeChanges as test_module
        self._run_test(request, workspace, editor, test_module)
