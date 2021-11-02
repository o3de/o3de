"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

"""

# This suite consists of all test cases that are under development and have not been verified yet.
# Once they are verified, please move them to TestSuite_Active.py

import pytest
import os
import sys


sys.path.append(os.path.dirname(os.path.abspath(__file__)) + '/../automatedtesting_shared')

from base import TestAutomationBase

@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
class TestAutomation(TestAutomationBase):
    def _run_prefab_test(self, request, workspace, editor, test_module, batch_mode=True, autotest_mode=True):
        self._run_test(request, workspace, editor, test_module, 
            extra_cmdline_args=["--regset=/Amazon/Preferences/EnablePrefabSystem=true"], 
            batch_mode=batch_mode,
            autotest_mode=autotest_mode)

    def test_Multiplayer_AutoComponent_NetworkInput(self, request, workspace, editor, launcher_platform):
        from .tests import Multiplayer_AutoComponent_NetworkInput as test_module
        self._run_prefab_test(request, workspace, editor, test_module)

