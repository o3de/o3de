"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
import pytest
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)) + '/../automatedtesting_shared')
from base import TestAutomationBase


@pytest.mark.SUITE_sandbox
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomation(TestAutomationBase):

    def test_Menus_EditMenuOptions_Work(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import Menus_EditMenuOptions as test_module
        self._run_test(request, workspace, editor, test_module, batch_mode=False, enable_prefab_system=False)

    def test_Docking_BasicDockedTools(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import Docking_BasicDockedTools as test_module
        self._run_test(request, workspace, editor, test_module, batch_mode=False, enable_prefab_system=False)
