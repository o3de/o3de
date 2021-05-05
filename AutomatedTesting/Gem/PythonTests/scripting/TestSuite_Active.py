"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

import pytest
import os
import sys

from ly_test_tools import LAUNCHERS

sys.path.append(os.path.dirname(os.path.abspath(__file__)) + '/../automatedtesting_shared')

from base import TestAutomationBase

@pytest.mark.SUITE_periodic
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomation(TestAutomationBase):
    
    def test_Docking_Pane(self, request, workspace, editor, launcher_platform):
        from . import Docking_Pane as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_Resizing_Pane(self, request, workspace, editor, launcher_platform):
        from . import Resizing_Pane as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_Unpin_VariableManager(self, request, workspace, editor, launcher_platform):
        from . import Unpin_VariableManager as test_module
        self._run_test(request, workspace, editor, test_module)
