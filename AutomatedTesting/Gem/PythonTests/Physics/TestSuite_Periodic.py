"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

"""

# This suite consists of all test cases that are passing and have been verified.

import pytest
import os
import sys

from .utils.FileManagement import FileManagement as fm
from ly_test_tools.o3de.editor_test import EditorSingleTest, EditorBatchedTest, EditorTestSuite

sys.path.append(os.path.dirname(os.path.abspath(__file__)) + '/../automatedtesting_shared')

from base import TestAutomationBase

revert_physics_config = fm.file_revert_list(['physxdebugconfiguration.setreg', 'physxdefaultsceneconfiguration.setreg', 'physxsystemconfiguration.setreg'], 'AutomatedTesting/Registry')

@pytest.mark.SUITE_periodic
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomation(TestAutomationBase):

    @revert_physics_config
    @fm.file_override('physxdefaultsceneconfiguration.setreg','ScriptCanvas_PostUpdateEvent.setreg_override',
                      'AutomatedTesting/Registry', search_subdirs=True)
    def test_ScriptCanvas_PostUpdateEvent(self, request, workspace, editor, launcher_platform):
        from .tests.script_canvas import ScriptCanvas_PostUpdateEvent as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    @fm.file_override('physxdefaultsceneconfiguration.setreg', 'ScriptCanvas_PreUpdateEvent.setreg_override',
                      'AutomatedTesting/Registry', search_subdirs=True)
    def test_ScriptCanvas_PreUpdateEvent(self, request, workspace, editor, launcher_platform):
        from .tests.script_canvas import ScriptCanvas_PreUpdateEvent as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_ScriptCanvas_SpawnEntityWithPhysComponents(self, request, workspace, editor, launcher_platform):
        from .tests.script_canvas import ScriptCanvas_SpawnEntityWithPhysComponents as test_module
        self._run_test(request, workspace, editor, test_module)
