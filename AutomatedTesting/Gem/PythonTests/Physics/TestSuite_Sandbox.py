"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

"""

import pytest
import os
import sys

from .utils.FileManagement import FileManagement as fm
from ly_test_tools.o3de.editor_test import EditorSingleTest, EditorBatchedTest, EditorTestSuite

sys.path.append(os.path.dirname(os.path.abspath(__file__)) + '/../automatedtesting_shared')

from base import TestAutomationBase

revert_physics_config = fm.file_revert_list(['physxdebugconfiguration.setreg', 'physxdefaultsceneconfiguration.setreg', 'physxsystemconfiguration.setreg'], 'AutomatedTesting/Registry')

@pytest.mark.SUITE_sandbox
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomation(TestAutomationBase):

    ## Seems to be flaky, need to investigate
    def test_ScriptCanvas_GetCollisionNameReturnsName(self, request, workspace, editor, launcher_platform):
        from .tests.script_canvas import ScriptCanvas_GetCollisionNameReturnsName as test_module
        # Fixme: expected_lines=["Layer Name: Right"]
        self._run_test(request, workspace, editor, test_module)

    ## Seems to be flaky, need to investigate
    def test_ScriptCanvas_GetCollisionNameReturnsNothingWhenHasToggledLayer(self, request, workspace, editor, launcher_platform):
        from .tests.script_canvas import ScriptCanvas_GetCollisionNameReturnsNothingWhenHasToggledLayer as test_module
        # All groups present in the PhysX Collider that could show up in test
        # Fixme: collision_groups = ["All", "None", "All_NoTouchBend", "All_3", "None_1", "All_NoTouchBend_1", "All_2", "None_1_1", "All_NoTouchBend_1_1", "All_1", "None_1_1_1", "All_NoTouchBend_1_1_1", "All_4", "None_1_1_1_1", "All_NoTouchBend_1_1_1_1", "GroupLeft", "GroupRight"]
        # Fixme: for group in collision_groups:
        # Fixme:    unexpected_lines.append(f"GroupName: {group}")
        # Fixme: expected_lines=["GroupName: "]
        self._run_test(request, workspace, editor, test_module)

@pytest.mark.SUITE_main
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class EditorTestAutomation(EditorTestSuite):
    global_extra_cmdline_args = ['-BatchMode', '-autotest_mode']

    @staticmethod
    def get_number_parallel_editors():
        return 16

    class CharacterController_SwitchLevels(EditorBatchedTest):
        from .tests.character_controller import CharacterController_SwitchLevels as test_module

    class ShapeCollider_CylinderShapeCollides(EditorBatchedTest):
        from .tests.shape_collider import ShapeCollider_CylinderShapeCollides as test_module

    class Collider_SphereShapeEditing(EditorBatchedTest):
        from .tests.collider import Collider_SphereShapeEditing as test_module

    class Collider_PxMeshAutoAssignedWhenAddingRenderMeshComponent(EditorBatchedTest):
        from .tests.collider import Collider_PxMeshAutoAssignedWhenAddingRenderMeshComponent as test_module

    class Collider_PxMeshConvexMeshCollides(EditorBatchedTest):
        from .tests.collider import Collider_PxMeshConvexMeshCollides as test_module

    class ShapeCollider_CanBeAddedWitNoWarnings(EditorBatchedTest):
        from .tests.shape_collider import ShapeCollider_CanBeAddedWitNoWarnings as test_module

    class ShapeCollider_InactiveWhenNoShapeComponent(EditorBatchedTest):
        from .tests.shape_collider import ShapeCollider_InactiveWhenNoShapeComponent as test_module

    class ShapeCollider_LargeNumberOfShapeCollidersWontCrashEditor(EditorBatchedTest):
        from .tests.shape_collider import ShapeCollider_LargeNumberOfShapeCollidersWontCrashEditor as test_module

    class RigidBody_InitialLinearVelocity(EditorBatchedTest):
        from .tests.rigid_body import RigidBody_InitialLinearVelocity as test_module

    class RigidBody_StartGravityEnabledWorks(EditorBatchedTest):
        from .tests.rigid_body import RigidBody_StartGravityEnabledWorks as test_module

    class RigidBody_KinematicModeWorks(EditorBatchedTest):
        from .tests.rigid_body import RigidBody_KinematicModeWorks as test_module

    class RigidBody_MomentOfInertiaManualSetting(EditorBatchedTest):
        from .tests.rigid_body import RigidBody_MomentOfInertiaManualSetting as test_module

    class ScriptCanvas_TriggerEvents(EditorBatchedTest):
        from .tests.script_canvas import ScriptCanvas_TriggerEvents as test_module
        # needs to be updated to log for unexpected lines
        # expected_lines = test_module.LogLines.expected_lines

    class ScriptCanvas_CollisionEvents(EditorBatchedTest):
        from .tests.script_canvas import ScriptCanvas_CollisionEvents as test_module

    class RigidBody_StartAsleepWorks(EditorBatchedTest):
        from .tests.rigid_body import RigidBody_StartAsleepWorks as test_module

    class RigidBody_MassDifferentValuesWorks(EditorBatchedTest):
        from .tests.rigid_body import RigidBody_MassDifferentValuesWorks as test_module

    class Material_NoEffectIfNoColliderShape(EditorBatchedTest):
        from .tests.material import Material_NoEffectIfNoColliderShape as test_module

    class RigidBody_SetGravityWorks(EditorBatchedTest):
        from .tests.rigid_body import RigidBody_SetGravityWorks as test_module

    class Material_EmptyLibraryUsesDefault(EditorBatchedTest):
        from .tests.material import Material_EmptyLibraryUsesDefault as test_module

    class ScriptCanvas_PostPhysicsUpdate(EditorBatchedTest):
        from .tests.script_canvas import ScriptCanvas_PostPhysicsUpdate as test_module
        # Note: Test needs to be updated to log for unexpected lines
        # unexpected_lines = ["Assert"] + test_module.Lines.unexpected

    class ForceRegion_PrefabFileInstantiates(EditorBatchedTest):
        from .tests.force_region import ForceRegion_PrefabFileInstantiates as test_module
