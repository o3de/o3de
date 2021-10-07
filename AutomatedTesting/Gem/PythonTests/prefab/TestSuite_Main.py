"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

 """

# This suite consists of all test cases that are passing and have been verified.

import pytest
import os
import sys

from ly_test_tools import LAUNCHERS
sys.path.append(os.path.dirname(os.path.abspath(__file__)) + '/../automatedtesting_shared')

from base import TestAutomationBase

@pytest.mark.SUITE_main
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomation(TestAutomationBase):

    def _run_prefab_test(self, request, workspace, editor, test_module, batch_mode=True, autotest_mode=True):
        self._run_test(request, workspace, editor, test_module, 
            extra_cmdline_args=["--regset=/Amazon/Preferences/EnablePrefabSystem=true"], 
            batch_mode=batch_mode,
            autotest_mode=autotest_mode)

    def test_PrefabLevel_OpensLevelWithEntities(self, request, workspace, editor, launcher_platform):
        from . import PrefabLevel_OpensLevelWithEntities as test_module
        self._run_prefab_test(request, workspace, editor, test_module)

    def test_Prefab_BasicWorkflow_CreatePrefab(self, request, workspace, editor, launcher_platform):
        from . import Prefab_BasicWorkflow_CreatePrefab as test_module
        self._run_prefab_test(request, workspace, editor, test_module)
        
    def test_Prefab_BasicWorkflow_InstantiatePrefab(self, request, workspace, editor, launcher_platform):
        from . import Prefab_BasicWorkflow_InstantiatePrefab as test_module
        self._run_prefab_test(request, workspace, editor, test_module)

    def test_Prefab_BasicWorkflow_CreateAndDeletePrefab(self, request, workspace, editor, launcher_platform):
        from . import Prefab_BasicWorkflow_CreateAndDeletePrefab as test_module
        self._run_prefab_test(request, workspace, editor, test_module)

    def test_Prefab_BasicWorkflow_CreateAndReparentPrefab(self, request, workspace, editor, launcher_platform):
        from . import Prefab_BasicWorkflow_CreateAndReparentPrefab as test_module
        self._run_prefab_test(request, workspace, editor, test_module, autotest_mode=False)
