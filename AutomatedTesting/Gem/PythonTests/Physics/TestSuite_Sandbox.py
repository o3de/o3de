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


# Custom test spec, it provides functionality to override files
class EditorSingleTest_WithFileOverrides(EditorSingleTest):
    # Specify here what files to override, [(original, override), ...]
    files_to_override = [()]
    # Base directory of the files (Default path is {ProjectName})
    base_dir = None
    # True will will search sub-directories for the files in base
    search_subdirs = True

    @classmethod
    def wrap_run(cls, instance, request, workspace, editor_test_results, launcher_platform):
        root_path = cls.base_dir
        if root_path is not None:
            root_path = os.path.join(workspace.paths.engine_root(), root_path)
        else:
            # Default to project folder
            root_path = workspace.paths.project()

        # Try to locate both target and source files
        original_file_list, override_file_list = zip(*cls.files_to_override)
        try:
            file_list = fm._find_files(original_file_list + override_file_list, root_path, cls.search_subdirs)
        except RuntimeWarning as w:
            assert False, (
                    w.message
                    + " Please check use of search_subdirs; make sure you are using the correct parent directory."
            )

        for f in original_file_list:
            fm._restore_file(f, file_list[f])
            fm._backup_file(f, file_list[f])

        for original, override in cls.files_to_override:
            fm._copy_file(override, file_list[override], original, file_list[original])

        yield  # Run Test
        for f in original_file_list:
            fm._restore_file(f, file_list[f])


@pytest.mark.SUITE_main
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class EditorTestAutomation(EditorTestSuite):
    global_extra_cmdline_args = ['-BatchMode', '-autotest_mode']

    @staticmethod
    def get_number_parallel_editors():
        return 16

    #########################################
    # Non-atomic tests: These need to be run in a single editor because they have custom setup and teardown
    class Material_DynamicFriction(EditorSingleTest_WithFileOverrides):
        from .tests.material import Material_DynamicFriction as test_module
        files_to_override = [
            ('physxsystemconfiguration.setreg', 'Material_DynamicFriction.setreg_override')
        ]
        base_dir = "AutomatedTesting/Registry"

    @pytest.mark.skip(reason="GHI #9422: Test Periodically Fails")
    class Collider_DiffCollisionGroupDiffCollidingLayersNotCollide(EditorSingleTest_WithFileOverrides):
        from .tests.collider import Collider_DiffCollisionGroupDiffCollidingLayersNotCollide as test_module
        files_to_override = [
            ('physxsystemconfiguration.setreg',
             'Collider_DiffCollisionGroupDiffCollidingLayersNotCollide.setreg_override')
        ]
        base_dir = "AutomatedTesting/Registry"

    #########################################

    class RigidBody_EnablingGravityWorksUsingNotificationsPoC(EditorBatchedTest):
        from .tests.rigid_body import RigidBody_EnablingGravityWorksUsingNotificationsPoC as test_module

    class ForceRegion_LocalSpaceForceOnRigidBodies(EditorBatchedTest):
        from .tests.force_region import ForceRegion_LocalSpaceForceOnRigidBodies as test_module

    class Collider_SameCollisionGroupDiffLayersCollide(EditorBatchedTest):
        from .tests.collider import Collider_SameCollisionGroupDiffLayersCollide as test_module

    class CharacterController_SwitchLevels(EditorBatchedTest):
        from .tests.character_controller import CharacterController_SwitchLevels as test_module

    class Ragdoll_AddPhysxRagdollComponentWorks(EditorBatchedTest):
        from .tests.ragdoll import Ragdoll_AddPhysxRagdollComponentWorks as test_module

    class ScriptCanvas_MultipleRaycastNode(EditorBatchedTest):
        # Fixme: This test previously relied on unexpected lines log reading with is now not supported.
        # Now the log reading must be done inside the test, preferably with the Tracer() utility
        #  unexpected_lines = ["Assert"] + test_module.Lines.unexpected
        from .tests.script_canvas import ScriptCanvas_MultipleRaycastNode as test_module

    class Joints_HingeLeadFollowerCollide(EditorBatchedTest):
        from .tests.joints import Joints_HingeLeadFollowerCollide as test_module

    class ShapeCollider_CylinderShapeCollides(EditorBatchedTest):
        from .tests.shape_collider import ShapeCollider_CylinderShapeCollides as test_module

    @pytest.mark.GROUP_tick
    @pytest.mark.xfail(reason="Test still under development.")
    class Tick_InterpolatedRigidBodyMotionIsSmooth(EditorBatchedTest):
        from .tests.tick import Tick_InterpolatedRigidBodyMotionIsSmooth as test_module

    @pytest.mark.GROUP_tick
    @pytest.mark.xfail(reason="Test still under development.")
    class Tick_CharacterGameplayComponentMotionIsSmooth(EditorBatchedTest):
        from .tests.tick import Tick_CharacterGameplayComponentMotionIsSmooth as test_module

    class Collider_BoxShapeEditing(EditorBatchedTest):
        from .tests.collider import Collider_BoxShapeEditing as test_module

    class Collider_SphereShapeEditing(EditorBatchedTest):
        from .tests.collider import Collider_SphereShapeEditing as test_module

    class Collider_CapsuleShapeEditing(EditorBatchedTest):
        from .tests.collider import Collider_CapsuleShapeEditing as test_module

    class Collider_CheckDefaultShapeSettingIsPxMesh(EditorBatchedTest):
        from .tests.collider import Collider_CheckDefaultShapeSettingIsPxMesh as test_module

    class Collider_MultipleSurfaceSlots(EditorBatchedTest):
        from .tests.collider import Collider_MultipleSurfaceSlots as test_module

    class Collider_PxMeshAutoAssignedWhenAddingRenderMeshComponent(EditorBatchedTest):
        from .tests.collider import Collider_PxMeshAutoAssignedWhenAddingRenderMeshComponent as test_module

    @pytest.mark.xfail(reason="AssertionError: Couldn't find Asset with path: Objects/SphereBot/r0-b_body.azmodel")
    class Collider_PxMeshAutoAssignedWhenModifyingRenderMeshComponent(EditorBatchedTest):
        from .tests.collider import Collider_PxMeshAutoAssignedWhenModifyingRenderMeshComponent as test_module

    class Collider_PxMeshConvexMeshCollides(EditorBatchedTest):
        from .tests.collider import Collider_PxMeshConvexMeshCollides as test_module

    class Material_LibraryClearingAssignsDefault(EditorBatchedTest):
        from .tests.material import Material_LibraryClearingAssignsDefault as test_module

    class ShapeCollider_CanBeAddedWitNoWarnings(EditorBatchedTest):
        from .tests.shape_collider import ShapeCollider_CanBeAddedWitNoWarnings as test_module

    class ShapeCollider_InactiveWhenNoShapeComponent(EditorBatchedTest):
        from .tests.shape_collider import ShapeCollider_InactiveWhenNoShapeComponent as test_module

    class ShapeCollider_LargeNumberOfShapeCollidersWontCrashEditor(EditorBatchedTest):
        from .tests.shape_collider import ShapeCollider_LargeNumberOfShapeCollidersWontCrashEditor as test_module

    class ForceRegion_WithNonTriggerColliderWarning(EditorBatchedTest):
        from .tests.force_region import ForceRegion_WithNonTriggerColliderWarning as test_module
        # Fixme: expected_lines = ["[Warning] (PhysX Force Region) - Please ensure collider component marked as trigger exists in entity"]

    @pytest.mark.skip(reason="GHI #9301: Test Periodically Fails")
    class Collider_PxMeshNotAutoAssignedWhenNoPhysicsFbx(EditorBatchedTest):
        from .tests.collider import Collider_PxMeshNotAutoAssignedWhenNoPhysicsFbx as test_module

    class Collider_AddColliderComponent(EditorBatchedTest):
        from .tests.collider import Collider_AddColliderComponent as test_module

    class Terrain_NoPhysTerrainComponentNoCollision(EditorBatchedTest):
        from .tests.terrain import Terrain_NoPhysTerrainComponentNoCollision as test_module

    class RigidBody_InitialLinearVelocity(EditorBatchedTest):
        from .tests.rigid_body import RigidBody_InitialLinearVelocity as test_module

    class RigidBody_StartGravityEnabledWorks(EditorBatchedTest):
        from .tests.rigid_body import RigidBody_StartGravityEnabledWorks as test_module

    class RigidBody_KinematicModeWorks(EditorBatchedTest):
        from .tests.rigid_body import RigidBody_KinematicModeWorks as test_module

    @pytest.mark.skip(reason="GHI #9364: Test Periodically Fails")
    class ForceRegion_LinearDampingForceOnRigidBodies(EditorBatchedTest):
        from .tests.force_region import ForceRegion_LinearDampingForceOnRigidBodies as test_module

    class ForceRegion_SimpleDragForceOnRigidBodies(EditorBatchedTest):
        from .tests.force_region import ForceRegion_SimpleDragForceOnRigidBodies as test_module

    class ForceRegion_CapsuleShapedForce(EditorBatchedTest):
        from .tests.force_region import ForceRegion_CapsuleShapedForce as test_module

    class ForceRegion_ImpulsesCapsuleShapedRigidBody(EditorBatchedTest):
        from .tests.force_region import ForceRegion_ImpulsesCapsuleShapedRigidBody as test_module

    class RigidBody_MomentOfInertiaManualSetting(EditorBatchedTest):
        from .tests.rigid_body import RigidBody_MomentOfInertiaManualSetting as test_module

    class RigidBody_COM_ManualSettingWorks(EditorBatchedTest):
        from .tests.rigid_body import RigidBody_COM_ManualSettingWorks as test_module

    class RigidBody_AddRigidBodyComponent(EditorBatchedTest):
        from .tests.rigid_body import RigidBody_AddRigidBodyComponent as test_module

    class ForceRegion_SplineForceOnRigidBodies(EditorBatchedTest):
        from .tests.force_region import ForceRegion_SplineForceOnRigidBodies as test_module

    class Collider_ColliderPositionOffset(EditorBatchedTest):
        from .tests.collider import Collider_ColliderPositionOffset as test_module

    class RigidBody_AngularDampingAffectsRotation(EditorBatchedTest):
        from .tests.rigid_body import RigidBody_AngularDampingAffectsRotation as test_module

    class Physics_VerifyColliderRigidBodyMeshAndTerrainWorkTogether(EditorBatchedTest):
        from .tests import Physics_VerifyColliderRigidBodyMeshAndTerrainWorkTogether as test_module

    class ForceRegion_MultipleForcesInSameComponentCombineForces(EditorBatchedTest):
        from .tests.force_region import ForceRegion_MultipleForcesInSameComponentCombineForces as test_module

    class ForceRegion_ImpulsesPxMeshShapedRigidBody(EditorBatchedTest):
        from .tests.force_region import ForceRegion_ImpulsesPxMeshShapedRigidBody as test_module

    class ScriptCanvas_TriggerEvents(EditorBatchedTest):
        from .tests.script_canvas import ScriptCanvas_TriggerEvents as test_module
        # needs to be updated to log for unexpected lines
        # expected_lines = test_module.LogLines.expected_lines

    class ForceRegion_ZeroPointForceDoesNothing(EditorBatchedTest):
        from .tests.force_region import ForceRegion_ZeroPointForceDoesNothing as test_module

    class ForceRegion_ZeroWorldSpaceForceDoesNothing(EditorBatchedTest):
        from .tests.force_region import ForceRegion_ZeroWorldSpaceForceDoesNothing as test_module

    class ForceRegion_ZeroLinearDampingDoesNothing(EditorBatchedTest):
        from .tests.force_region import ForceRegion_ZeroLinearDampingDoesNothing as test_module

    class ForceRegion_MovingForceRegionChangesNetForce(EditorBatchedTest):
        from .tests.force_region import ForceRegion_MovingForceRegionChangesNetForce as test_module

    class ScriptCanvas_CollisionEvents(EditorBatchedTest):
        from .tests.script_canvas import ScriptCanvas_CollisionEvents as test_module

    class ForceRegion_DirectionHasNoAffectOnTotalForce(EditorBatchedTest):
        from .tests.force_region import ForceRegion_DirectionHasNoAffectOnTotalForce as test_module

    class RigidBody_StartAsleepWorks(EditorBatchedTest):
        from .tests.rigid_body import RigidBody_StartAsleepWorks as test_module

    class ForceRegion_ZeroLocalSpaceForceDoesNothing(EditorBatchedTest):
        from .tests.force_region import ForceRegion_ZeroLocalSpaceForceDoesNothing as test_module

    class ForceRegion_ZeroSimpleDragForceDoesNothing(EditorBatchedTest):
        from .tests.force_region import ForceRegion_ZeroSimpleDragForceDoesNothing as test_module

    class RigidBody_COM_ComputingWorks(EditorBatchedTest):
        from .tests.rigid_body import RigidBody_COM_ComputingWorks as test_module

    class RigidBody_MassDifferentValuesWorks(EditorBatchedTest):
        from .tests.rigid_body import RigidBody_MassDifferentValuesWorks as test_module

    class ForceRegion_SplineRegionWithModifiedTransform(EditorBatchedTest):
        from .tests.force_region import ForceRegion_SplineRegionWithModifiedTransform as test_module

    class RigidBody_InitialAngularVelocity(EditorBatchedTest):
        from .tests.rigid_body import RigidBody_InitialAngularVelocity as test_module

    class ForceRegion_ZeroSplineForceDoesNothing(EditorBatchedTest):
        from .tests.force_region import ForceRegion_ZeroSplineForceDoesNothing as test_module

    class ForceRegion_PositionOffset(EditorBatchedTest):
        from .tests.force_region import ForceRegion_PositionOffset as test_module

    class Ragdoll_LevelSwitchDoesNotCrash(EditorBatchedTest):
        from .tests.ragdoll import Ragdoll_LevelSwitchDoesNotCrash as test_module

    class ForceRegion_MultipleComponentsCombineForces(EditorBatchedTest):
        from .tests.force_region import ForceRegion_MultipleComponentsCombineForces as test_module

    @pytest.mark.xfail(
        reason="This test will sometimes fail as the ball will continue to roll before the timeout is reached.")
    class RigidBody_SleepWhenBelowKineticThreshold(EditorBatchedTest):
        from .tests.rigid_body import RigidBody_SleepWhenBelowKineticThreshold as test_module

    class RigidBody_COM_NotIncludesTriggerShapes(EditorBatchedTest):
        from .tests.rigid_body import RigidBody_COM_NotIncludesTriggerShapes as test_module

    class Material_NoEffectIfNoColliderShape(EditorBatchedTest):
        from .tests.material import Material_NoEffectIfNoColliderShape as test_module

    @pytest.mark.xfail(reason="GHI #9579: Test periodically fails")
    class Collider_TriggerPassThrough(EditorBatchedTest):
        from .tests.collider import Collider_TriggerPassThrough as test_module

    class RigidBody_SetGravityWorks(EditorBatchedTest):
        from .tests.rigid_body import RigidBody_SetGravityWorks as test_module

    class Material_EmptyLibraryUsesDefault(EditorBatchedTest):
        from .tests.material import Material_EmptyLibraryUsesDefault as test_module

    @pytest.mark.skip(reason="GHI #9365: Test periodically fails")
    class ForceRegion_NoQuiverOnHighLinearDampingForce(EditorBatchedTest):
        from .tests.force_region import ForceRegion_NoQuiverOnHighLinearDampingForce as test_module

    @pytest.mark.xfail(reason="GHI #9565: Test periodically fails")
    class RigidBody_ComputeInertiaWorks(EditorBatchedTest):
        from .tests.rigid_body import RigidBody_ComputeInertiaWorks as test_module

    class ScriptCanvas_PostPhysicsUpdate(EditorBatchedTest):
        from .tests.script_canvas import ScriptCanvas_PostPhysicsUpdate as test_module
        # Note: Test needs to be updated to log for unexpected lines
        # unexpected_lines = ["Assert"] + test_module.Lines.unexpected

    class ForceRegion_PxMeshShapedForce(EditorBatchedTest):
        from .tests.force_region import ForceRegion_PxMeshShapedForce as test_module

    # Marking the Test as expected to fail using the xfail decorator due to sporadic failure on Automated Review: SPEC-3146
    # The test still runs, but a failure of the test doesn't result in the test run failing
    @pytest.mark.xfail(
        reason="Test Sporadically fails with message [ NOT FOUND ] Success: Bar1 : Expected angular velocity")
    class RigidBody_MaxAngularVelocityWorks(EditorBatchedTest):
        from .tests.rigid_body import RigidBody_MaxAngularVelocityWorks as test_module

    class Joints_HingeSoftLimitsConstrained(EditorBatchedTest):
        from .tests.joints import Joints_HingeSoftLimitsConstrained as test_module

    class Joints_BallLeadFollowerCollide(EditorBatchedTest):
        from .tests.joints import Joints_BallLeadFollowerCollide as test_module

    @pytest.mark.xfail(reason="GHI #9582: Test periodically fails")
    class ForceRegion_WorldSpaceForceOnRigidBodies(EditorBatchedTest):
        from .tests.force_region import ForceRegion_WorldSpaceForceOnRigidBodies as test_module

    @pytest.mark.xfail(reason="GHI #9566: Test periodically fails")
    class ForceRegion_PointForceOnRigidBodies(EditorBatchedTest):
        from .tests.force_region import ForceRegion_PointForceOnRigidBodies as test_module

    class ForceRegion_SphereShapedForce(EditorBatchedTest):
        from .tests.force_region import ForceRegion_SphereShapedForce as test_module

    class ForceRegion_RotationalOffset(EditorBatchedTest):
        from .tests.force_region import ForceRegion_RotationalOffset as test_module

    class RigidBody_EnablingGravityWorksPoC(EditorBatchedTest):
        from .tests.rigid_body import RigidBody_EnablingGravityWorksPoC as test_module

    @pytest.mark.xfail(reason="GHI #9368: Test Sporadically Fails")
    class Collider_ColliderRotationOffset(EditorBatchedTest):
        from .tests.collider import Collider_ColliderRotationOffset as test_module

    class ForceRegion_ParentChildForcesCombineForces(EditorBatchedTest):
        from .tests.force_region import ForceRegion_ParentChildForcesCombineForces as test_module

    class Physics_UndoRedoWorksOnEntityWithPhysComponents(EditorBatchedTest):
        from .tests import Physics_UndoRedoWorksOnEntityWithPhysComponents as test_module

    class ScriptCanvas_ShapeCast(EditorBatchedTest):
        from .tests.script_canvas import ScriptCanvas_ShapeCast as test_module

    class ForceRegion_PrefabFileInstantiates(EditorBatchedTest):
        from .tests.force_region import ForceRegion_PrefabFileInstantiates as test_module