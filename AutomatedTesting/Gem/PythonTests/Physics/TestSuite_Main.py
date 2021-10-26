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
from ly_test_tools import LAUNCHERS

sys.path.append(os.path.dirname(os.path.abspath(__file__)) + '/../automatedtesting_shared')

from base import TestAutomationBase


revert_physics_config = fm.file_revert_list(['physxdebugconfiguration.setreg', 'physxdefaultsceneconfiguration.setreg', 'physxsystemconfiguration.setreg'], 'AutomatedTesting/Registry')


@pytest.mark.SUITE_main
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomation(TestAutomationBase):

    def test_RigidBody_EnablingGravityWorksUsingNotificationsPoC(self, request, workspace, editor, launcher_platform):
        from .tests.rigid_body import RigidBody_EnablingGravityWorksUsingNotificationsPoC as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_ForceRegion_LocalSpaceForceOnRigidBodies(self, request, workspace, editor, launcher_platform):
        from .tests.force_region import ForceRegion_LocalSpaceForceOnRigidBodies as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    @fm.file_override('physxsystemconfiguration.setreg','Material_DynamicFriction.setreg_override',
                      'AutomatedTesting/Registry', search_subdirs=True)
    def test_Material_DynamicFriction(self, request, workspace, editor, launcher_platform):
        from .tests.material import Material_DynamicFriction as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_Collider_SameCollisionGroupDiffLayersCollide(self, request, workspace, editor, launcher_platform):
        from .tests.collider import Collider_SameCollisionGroupDiffLayersCollide as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_CharacterController_SwitchLevels(self, request, workspace, editor, launcher_platform):
        from .tests.character_controller import CharacterController_SwitchLevels as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_Ragdoll_AddPhysxRagdollComponentWorks(self, request, workspace, editor, launcher_platform):
        from .tests.ragdoll import Ragdoll_AddPhysxRagdollComponentWorks as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_ScriptCanvas_MultipleRaycastNode(self, request, workspace, editor, launcher_platform):
        from .tests.script_canvas import ScriptCanvas_MultipleRaycastNode as test_module
        # Fixme: This test previously relied on unexpected lines log reading with is now not supported.
        # Now the log reading must be done inside the test, preferably with the Tracer() utility
        #  unexpected_lines = ["Assert"] + test_module.Lines.unexpected
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    @fm.file_override('physxsystemconfiguration.setreg','Collider_DiffCollisionGroupDiffCollidingLayersNotCollide.setreg_override',
                      'AutomatedTesting/Registry', search_subdirs=True)
    def test_Collider_DiffCollisionGroupDiffCollidingLayersNotCollide(self, request, workspace, editor, launcher_platform):
        from .tests.collider import Collider_DiffCollisionGroupDiffCollidingLayersNotCollide as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_Joints_HingeLeadFollowerCollide(self, request, workspace, editor, launcher_platform):
        from .tests.joints import Joints_HingeLeadFollowerCollide as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_Collider_PxMeshConvexMeshCollides(self, request, workspace, editor, launcher_platform):
        from .tests.collider import Collider_PxMeshConvexMeshCollides as test_module
        self._run_test(request, workspace, editor, test_module)
    
    @revert_physics_config    
    def test_ShapeCollider_CylinderShapeCollides(self, request, workspace, editor, launcher_platform):
        from .tests.shape_collider import ShapeCollider_CylinderShapeCollides as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_C15425929_Undo_Redo(self, request, workspace, editor, launcher_platform):
        from .tests import Physics_UndoRedoWorksOnEntityWithPhysComponents as test_module
        self._run_test(request, workspace, editor, test_module)