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
    @fm.file_override('physxsystemconfiguration.setreg','Material_CharacterController.setreg_override',
                      'AutomatedTesting/Registry', search_subdirs=True)
    def test_Material_CharacterController(self, request, workspace, editor, launcher_platform):
        from .tests.material import Material_CharacterController as test_module
        self._run_test(request, workspace, editor, test_module)

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

    @revert_physics_config
    @fm.file_override('physxsystemconfiguration.setreg','Collider_AddingNewGroupWorks.setreg_override',
                      'AutomatedTesting/Registry', search_subdirs=True)
    def test_Collider_AddingNewGroupWorks(self, request, workspace, editor, launcher_platform):
        from .tests.collider import Collider_AddingNewGroupWorks as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    @fm.file_override('physxsystemconfiguration.setreg','Collider_CollisionGroupsWorkflow.setreg_override',
                      'AutomatedTesting/Registry', search_subdirs=True)
    def test_Collider_CollisionGroupsWorkflow(self, request, workspace, editor, launcher_platform):
        from .tests.collider import Collider_CollisionGroupsWorkflow as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_Joints_Fixed2BodiesConstrained(self, request, workspace, editor, launcher_platform):
        from .tests.joints import Joints_Fixed2BodiesConstrained as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_Joints_Hinge2BodiesConstrained(self, request, workspace, editor, launcher_platform):
        from .tests.joints import Joints_Hinge2BodiesConstrained as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_Joints_Ball2BodiesConstrained(self, request, workspace, editor, launcher_platform):
        from .tests.joints import Joints_Ball2BodiesConstrained as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_Joints_FixedBreakable(self, request, workspace, editor, launcher_platform):
        from .tests.joints import Joints_FixedBreakable as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_Joints_HingeBreakable(self, request, workspace, editor, launcher_platform):
        from .tests.joints import Joints_HingeBreakable as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_Joints_BallBreakable(self, request, workspace, editor, launcher_platform):
        from .tests.joints import Joints_BallBreakable as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_Joints_HingeNoLimitsConstrained(self, request, workspace, editor, launcher_platform):
        from .tests.joints import Joints_HingeNoLimitsConstrained as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_Joints_BallNoLimitsConstrained(self, request, workspace, editor, launcher_platform):
        from .tests.joints import Joints_BallNoLimitsConstrained as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_Joints_GlobalFrameConstrained(self, request, workspace, editor, launcher_platform):
        from .tests.joints import Joints_GlobalFrameConstrained as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_ScriptCanvas_SpawnEntityWithPhysComponents(self, request, workspace, editor, launcher_platform):
        from .tests.script_canvas import ScriptCanvas_SpawnEntityWithPhysComponents as test_module
        self._run_test(request, workspace, editor, test_module)
    
    @revert_physics_config
    def test_Material_DefaultLibraryUpdatedAcrossLevels(self, request, workspace, editor, launcher_platform):
        @fm.file_override("physxsystemconfiguration.setreg",
                          "Material_DefaultLibraryUpdatedAcrossLevels_before.setreg",
                          "AutomatedTesting",
                          search_subdirs=True)
        def levels_before(self, request, workspace, editor, launcher_platform):
            from .tests.material import Material_DefaultLibraryUpdatedAcrossLevels_before as test_module_0
            self._run_test(request, workspace, editor, test_module_0)

        # File override replaces the previous physxconfiguration file with another where the only difference is the default material library
        @fm.file_override("physxsystemconfiguration.setreg",
                          "Material_DefaultLibraryUpdatedAcrossLevels_after.setreg",
                          "AutomatedTesting",
                          search_subdirs=True)
        def levels_after(self, request, workspace, editor, launcher_platform):
            from .tests.material import Material_DefaultLibraryUpdatedAcrossLevels_after as test_module_1
            self._run_test(request, workspace, editor, test_module_1)

        levels_before(self, request, workspace, editor, launcher_platform)
        levels_after(self, request, workspace, editor, launcher_platform)
