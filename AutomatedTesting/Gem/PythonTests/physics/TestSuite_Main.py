"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

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


@pytest.mark.SUITE_main
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomation(TestAutomationBase):

    def test_C111111_RigidBody_EnablingGravityWorksUsingNotificationsPoC(self, request, workspace, editor, launcher_platform):
        from . import C111111_RigidBody_EnablingGravityWorksUsingNotificationsPoC as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_C5932041_PhysXForceRegion_LocalSpaceForceOnRigidBodies(self, request, workspace, editor, launcher_platform):
        from . import C5932041_PhysXForceRegion_LocalSpaceForceOnRigidBodies as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    @fm.file_override('physxsystemconfiguration.setreg','C4044459_Material_DynamicFriction.setreg_override', 'AutomatedTesting/Registry')
    def test_C4044459_Material_DynamicFriction(self, request, workspace, editor, launcher_platform):
        from . import C4044459_Material_DynamicFriction as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_C15425929_Undo_Redo(self, request, workspace, editor, launcher_platform):
        from . import C15425929_Undo_Redo as test_module
        self._run_test(request, workspace, editor, test_module)
        
    @revert_physics_config
    def test_C4976243_Collision_SameCollisionGroupDiffCollisionLayers(self, request, workspace, editor,
                                                                      launcher_platform):
        from . import C4976243_Collision_SameCollisionGroupDiffCollisionLayers as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_C14654881_CharacterController_SwitchLevels(self, request, workspace, editor, launcher_platform):
        from . import C14654881_CharacterController_SwitchLevels as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_C17411467_AddPhysxRagdollComponent(self, request, workspace, editor, launcher_platform):
        from . import C17411467_AddPhysxRagdollComponent as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_C12712453_ScriptCanvas_MultipleRaycastNode(self, request, workspace, editor, launcher_platform):
        from . import C12712453_ScriptCanvas_MultipleRaycastNode as test_module
        # Fixme: unexpected_lines = ["Assert"] + test_module.Lines.unexpected
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    @fm.file_override('physxsystemconfiguration.setreg','C4982593_PhysXCollider_CollisionLayer.setreg_override', 'AutomatedTesting/Registry')
    def test_C4982593_PhysXCollider_CollisionLayerTest(self, request, workspace, editor, launcher_platform):
        from . import C4982593_PhysXCollider_CollisionLayerTest as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_C18243586_Joints_HingeLeadFollowerCollide(self, request, workspace, editor, launcher_platform):
        from . import C18243586_Joints_HingeLeadFollowerCollide as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_C4982803_Enable_PxMesh_Option(self, request, workspace, editor, launcher_platform):
        from . import C4982803_Enable_PxMesh_Option as test_module
        self._run_test(request, workspace, editor, test_module)
    
    @revert_physics_config    
    def test_C24308873_CylinderShapeCollider_CollidesWithPhysXTerrain(self, request, workspace, editor, launcher_platform):
        from . import C24308873_CylinderShapeCollider_CollidesWithPhysXTerrain as test_module
        self._run_test(request, workspace, editor, test_module)