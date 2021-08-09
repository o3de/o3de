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

sys.path.append (os.path.dirname (os.path.abspath (__file__)) + '/../automatedtesting_shared')

import ly_test_tools.environment.file_system as file_system
from base import TestAutomationBase

@pytest.mark.SUITE_main
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomation(TestAutomationBase):

    def _run_prefab_test(self, request, workspace, editor, test_module):
        self._run_test(request, workspace, editor, test_module, ["--regset=/Amazon/Preferences/EnablePrefabSystem=true"])

    def test_PrefabLevel_OpensLevelWithEntities(self, request, workspace, editor, launcher_platform):
        from . import PrefabLevel_OpensLevelWithEntities as test_module
        self._run_prefab_test(request, workspace, editor, test_module)

    @pytest.mark.parametrize("level", ["tmp_level"])
    def test_PrefabLevel_BasicWorkflow(self, request, workspace, editor, launcher_platform, level):
        def teardown():
            file_system.delete([os.path.join(workspace.paths.project(), "Levels", level)], True, True)
        request.addfinalizer(teardown)
        file_system.delete([os.path.join(workspace.paths.project(), "Levels", level)], True, True)

        from . import PrefabLevel_BasicWorkflow as test_module
        self._run_prefab_test(request, workspace, editor, test_module)

