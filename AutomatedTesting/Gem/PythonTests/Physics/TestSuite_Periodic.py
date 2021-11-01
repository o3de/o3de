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


@pytest.mark.SUITE_periodic
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomation(TestAutomationBase):

    @revert_physics_config
    def test_Terrain_NoPhysTerrainComponentNoCollision(self, request, workspace, editor, launcher_platform):
        from .tests.terrain import Terrain_NoPhysTerrainComponentNoCollision as test_module
        self._run_test(request, workspace, editor, test_module)
        
    @revert_physics_config
    def test_RigidBody_InitialLinearVelocity(self, request, workspace, editor, launcher_platform):
        from .tests.rigid_body import RigidBody_InitialLinearVelocity as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_RigidBody_StartGravityEnabledWorks(self, request, workspace, editor, launcher_platform):
        from .tests.rigid_body import RigidBody_StartGravityEnabledWorks as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_RigidBody_KinematicModeWorks(self, request, workspace, editor, launcher_platform):
        from .tests.rigid_body import RigidBody_KinematicModeWorks as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_ForceRegion_LinearDampingForceOnRigidBodies(self, request, workspace, editor, launcher_platform):
        from .tests.force_region import ForceRegion_LinearDampingForceOnRigidBodies as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_ForceRegion_SimpleDragForceOnRigidBodies(self, request, workspace, editor, launcher_platform):
        from .tests.force_region import ForceRegion_SimpleDragForceOnRigidBodies as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_ForceRegion_CapsuleShapedForce(self, request, workspace, editor, launcher_platform):
        from .tests.force_region import ForceRegion_CapsuleShapedForce as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_ForceRegion_ImpulsesCapsuleShapedRigidBody(self, request, workspace, editor, launcher_platform):
        from .tests.force_region import ForceRegion_ImpulsesCapsuleShapedRigidBody as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_RigidBody_MomentOfInertiaManualSetting(self, request, workspace, editor, launcher_platform):
        from .tests.rigid_body import RigidBody_MomentOfInertiaManualSetting as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_RigidBody_COM_ManualSettingWorks(self, request, workspace, editor, launcher_platform):
        from .tests.rigid_body import RigidBody_COM_ManualSettingWorks as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_RigidBody_AddRigidBodyComponent(self, request, workspace, editor, launcher_platform):
        from .tests.rigid_body import RigidBody_AddRigidBodyComponent as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_ForceRegion_SplineForceOnRigidBodies(self, request, workspace, editor, launcher_platform):
        from .tests.force_region import ForceRegion_SplineForceOnRigidBodies as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    @fm.file_override('physxsystemconfiguration.setreg','Material_RestitutionCombine.setreg_override',
                      'AutomatedTesting/Registry', search_subdirs=True)
    def test_Material_RestitutionCombine(self, request, workspace, editor, launcher_platform):
        from .tests.material import Material_RestitutionCombine as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    @fm.file_override('physxsystemconfiguration.setreg','Material_FrictionCombine.setreg_override',
                      'AutomatedTesting/Registry', search_subdirs=True)
    def test_Material_FrictionCombine(self, request, workspace, editor, launcher_platform):
        from .tests.material import Material_FrictionCombine as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_Collider_ColliderPositionOffset(self, request, workspace, editor, launcher_platform):
        from .tests.collider import Collider_ColliderPositionOffset as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_RigidBody_AngularDampingAffectsRotation(self, request, workspace, editor, launcher_platform):
        from .tests.rigid_body import RigidBody_AngularDampingAffectsRotation as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_Physics_VerifyColliderRigidBodyMeshAndTerrainWorkTogether(self, request, workspace, editor, launcher_platform):
        from .tests import Physics_VerifyColliderRigidBodyMeshAndTerrainWorkTogether as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_ForceRegion_MultipleForcesInSameComponentCombineForces(self, request, workspace, editor, launcher_platform):
        from .tests.force_region import ForceRegion_MultipleForcesInSameComponentCombineForces as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_ForceRegion_ImpulsesPxMeshShapedRigidBody(self, request, workspace, editor, launcher_platform):
        from .tests.force_region import ForceRegion_ImpulsesPxMeshShapedRigidBody as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_ScriptCanvas_TriggerEvents(self, request, workspace, editor, launcher_platform):
        from .tests.script_canvas import ScriptCanvas_TriggerEvents as test_module
        # FIXME: expected_lines = test_module.LogLines.expected_lines
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_ForceRegion_ZeroPointForceDoesNothing(self, request, workspace, editor, launcher_platform):
        from .tests.force_region import ForceRegion_ZeroPointForceDoesNothing as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_ForceRegion_ZeroWorldSpaceForceDoesNothing(self, request, workspace, editor, launcher_platform):
        from .tests.force_region import ForceRegion_ZeroWorldSpaceForceDoesNothing as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_ForceRegion_ZeroLinearDampingDoesNothing(self, request, workspace, editor, launcher_platform):
        from .tests.force_region import ForceRegion_ZeroLinearDampingDoesNothing as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_ForceRegion_MovingForceRegionChangesNetForce(self, request, workspace, editor, launcher_platform):
        from .tests.force_region import ForceRegion_MovingForceRegionChangesNetForce as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_ScriptCanvas_CollisionEvents(self, request, workspace, editor, launcher_platform):
        from .tests.script_canvas import ScriptCanvas_CollisionEvents as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_ForceRegion_DirectionHasNoAffectOnTotalForce(self, request, workspace, editor, launcher_platform):
        from .tests.force_region import ForceRegion_DirectionHasNoAffectOnTotalForce as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_RigidBody_StartAsleepWorks(self, request, workspace, editor, launcher_platform):
        from .tests.rigid_body import RigidBody_StartAsleepWorks as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_ForceRegion_SliceFileInstantiates(self, request, workspace, editor, launcher_platform):
        from .tests.force_region import ForceRegion_SliceFileInstantiates as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_ForceRegion_ZeroLocalSpaceForceDoesNothing(self, request, workspace, editor, launcher_platform):
        from .tests.force_region import ForceRegion_ZeroLocalSpaceForceDoesNothing as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_ForceRegion_ZeroSimpleDragForceDoesNothing(self, request, workspace, editor, launcher_platform):
        from .tests.force_region import ForceRegion_ZeroSimpleDragForceDoesNothing as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_RigidBody_COM_ComputingWorks(self, request, workspace, editor, launcher_platform):
        from .tests.rigid_body import RigidBody_COM_ComputingWorks as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_RigidBody_MassDifferentValuesWorks(self, request, workspace, editor, launcher_platform):
        from .tests.rigid_body import RigidBody_MassDifferentValuesWorks as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    @fm.file_override('physxsystemconfiguration.setreg','Material_RestitutionCombinePriorityOrder.setreg_override',
                      'AutomatedTesting/Registry', search_subdirs=True)
    def test_Material_RestitutionCombinePriorityOrder(self, request, workspace, editor, launcher_platform):
        from .tests.material import Material_RestitutionCombinePriorityOrder as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_ForceRegion_SplineRegionWithModifiedTransform(self, request, workspace, editor, launcher_platform):
        from .tests.force_region import ForceRegion_SplineRegionWithModifiedTransform as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_ScriptCanvas_ShapeCast(self, request, workspace, editor, launcher_platform):
        from .tests.script_canvas import ScriptCanvas_ShapeCast as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_RigidBody_InitialAngularVelocity(self, request, workspace, editor, launcher_platform):
        from .tests.rigid_body import RigidBody_InitialAngularVelocity as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_ForceRegion_ZeroSplineForceDoesNothing(self, request, workspace, editor, launcher_platform):
        from .tests.force_region import ForceRegion_ZeroSplineForceDoesNothing as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_Physics_DynamicSliceWithPhysNotSpawnsStaticSlice(self, request, workspace, editor, launcher_platform):
        from .tests import Physics_DynamicSliceWithPhysNotSpawnsStaticSlice as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_ForceRegion_PositionOffset(self, request, workspace, editor, launcher_platform):
        from .tests.force_region import ForceRegion_PositionOffset as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    @fm.file_override('physxsystemconfiguration.setreg','Material_FrictionCombinePriorityOrder.setreg_override',
                      'AutomatedTesting/Registry', search_subdirs=True)
    def test_Material_FrictionCombinePriorityOrder(self, request, workspace, editor, launcher_platform):
        from .tests.material import Material_FrictionCombinePriorityOrder as test_module
        self._run_test(request, workspace, editor, test_module)

    @pytest.mark.xfail(
        reason="Something with the CryRenderer disabling is causing this test to fail now.")
    @revert_physics_config
    def test_Ragdoll_LevelSwitchDoesNotCrash(self, request, workspace, editor, launcher_platform):
        from .tests.ragdoll import Ragdoll_LevelSwitchDoesNotCrash as test_module
        self._run_test(request, workspace, editor, test_module)
    
    @revert_physics_config
    def test_ForceRegion_MultipleComponentsCombineForces(self, request, workspace, editor, launcher_platform):
        from .tests.force_region import ForceRegion_MultipleComponentsCombineForces as test_module
        self._run_test(request, workspace, editor, test_module)

    # Marking the test as an expected failure due to sporadic failure on Automated Review: LYN-2580
    # The test still runs, but a failure of the test doesn't result in the test run failing
    @pytest.mark.xfail(
        reason="This test needs new physics asset with multiple materials to be more stable.")
    @revert_physics_config
    @fm.file_override('physxsystemconfiguration.setreg', 'Material_PerFaceMaterialGetsCorrectMaterial.setreg_override',
                      'AutomatedTesting/Registry', search_subdirs=True)
    def test_Material_PerFaceMaterialGetsCorrectMaterial(self, request, workspace, editor, launcher_platform):
        from .tests.material import Material_PerFaceMaterialGetsCorrectMaterial as test_module
        self._run_test(request, workspace, editor, test_module)

    @pytest.mark.xfail(
        reason="This test will sometimes fail as the ball will continue to roll before the timeout is reached.")
    @revert_physics_config
    def test_RigidBody_SleepWhenBelowKineticThreshold(self, request, workspace, editor, launcher_platform):
        from .tests.rigid_body import RigidBody_SleepWhenBelowKineticThreshold as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_RigidBody_COM_NotIncludesTriggerShapes(self, request, workspace, editor, launcher_platform):
        from .tests.rigid_body import RigidBody_COM_NotIncludesTriggerShapes as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_Material_NoEffectIfNoColliderShape(self, request, workspace, editor, launcher_platform):
        from .tests.material import Material_NoEffectIfNoColliderShape as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_Collider_TriggerPassThrough(self, request, workspace, editor, launcher_platform):
        from .tests.collider import Collider_TriggerPassThrough as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_RigidBody_SetGravityWorks(self, request, workspace, editor, launcher_platform):
        from .tests.rigid_body import RigidBody_SetGravityWorks as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    @fm.file_override('physxsystemconfiguration.setreg','Material_CharacterController.setreg_override',
                      'AutomatedTesting/Registry', search_subdirs=True)
    def test_Material_CharacterController(self, request, workspace, editor, launcher_platform):
        from .tests.material import Material_CharacterController as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_Material_EmptyLibraryUsesDefault(self, request, workspace, editor, launcher_platform):
        from .tests.material import Material_EmptyLibraryUsesDefault as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_ForceRegion_NoQuiverOnHighLinearDampingForce(self, request, workspace, editor, launcher_platform):
        from .tests.force_region import ForceRegion_NoQuiverOnHighLinearDampingForce as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_RigidBody_ComputeInertiaWorks(self, request, workspace, editor, launcher_platform):
        from .tests.rigid_body import RigidBody_ComputeInertiaWorks as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_ScriptCanvas_PostPhysicsUpdate(self, request, workspace, editor, launcher_platform):
        from .tests.script_canvas import ScriptCanvas_PostPhysicsUpdate as test_module
        # Fixme: unexpected_lines = ["Assert"] + test_module.Lines.unexpected
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    @fm.file_override('physxsystemconfiguration.setreg','Collider_NoneCollisionGroupSameLayerNotCollide.setreg_override',
                      'AutomatedTesting/Registry', search_subdirs=True)
    def test_Collider_NoneCollisionGroupSameLayerNotCollide(self, request, workspace, editor, launcher_platform):
        from .tests.collider import Collider_NoneCollisionGroupSameLayerNotCollide as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    @fm.file_override('physxsystemconfiguration.setreg','Collider_SameCollisionGroupSameCustomLayerCollide.setreg_override',
                      'AutomatedTesting/Registry', search_subdirs=True)
    def test_Collider_SameCollisionGroupSameCustomLayerCollide(self, request, workspace, editor, launcher_platform):
        from .tests.collider import Collider_SameCollisionGroupSameCustomLayerCollide as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    @fm.file_override('physxdefaultsceneconfiguration.setreg','ScriptCanvas_PostUpdateEvent.setreg_override',
                      'AutomatedTesting/Registry', search_subdirs=True)
    def test_ScriptCanvas_PostUpdateEvent(self, request, workspace, editor, launcher_platform):
        from .tests.script_canvas import ScriptCanvas_PostUpdateEvent as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    @fm.file_override('physxsystemconfiguration.setreg','Material_Restitution.setreg_override',
                      'AutomatedTesting/Registry', search_subdirs=True)
    def test_Material_Restitution(self, request, workspace, editor, launcher_platform):
        from .tests.material import Material_Restitution as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    @fm.file_override('physxdefaultsceneconfiguration.setreg', 'ScriptCanvas_PreUpdateEvent.setreg_override',
                      'AutomatedTesting/Registry', search_subdirs=True)
    def test_ScriptCanvas_PreUpdateEvent(self, request, workspace, editor, launcher_platform):
        from .tests.script_canvas import ScriptCanvas_PreUpdateEvent as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_ForceRegion_PxMeshShapedForce(self, request, workspace, editor, launcher_platform):
        from .tests.force_region import ForceRegion_PxMeshShapedForce as test_module
        self._run_test(request, workspace, editor, test_module)

    # Marking the Test  as expected to fail using the xfail decorator due to sporadic failure on Automated Review: SPEC-3146
    # The test still runs, but a failure of the test doesn't result in the test run failing
    @pytest.mark.xfail(reason="Test Sporadically fails with message [ NOT FOUND ] Success: Bar1 : Expected angular velocity")
    @revert_physics_config
    def test_RigidBody_MaxAngularVelocityWorks(self, request, workspace, editor, launcher_platform):
        from .tests.rigid_body import RigidBody_MaxAngularVelocityWorks as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_Joints_HingeSoftLimitsConstrained(self, request, workspace, editor, launcher_platform):
        from .tests.joints import Joints_HingeSoftLimitsConstrained as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_Joints_BallSoftLimitsConstrained(self, request, workspace, editor, launcher_platform):
        from .tests.joints import Joints_BallSoftLimitsConstrained as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_Joints_BallLeadFollowerCollide(self, request, workspace, editor, launcher_platform):
        from .tests.joints import Joints_BallLeadFollowerCollide as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    @fm.file_override('physxsystemconfiguration.setreg','Collider_AddingNewGroupWorks.setreg_override',
                      'AutomatedTesting/Registry', search_subdirs=True)
    def test_Collider_AddingNewGroupWorks(self, request, workspace, editor, launcher_platform):
        from .tests.collider import Collider_AddingNewGroupWorks as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_ShapeCollider_InactiveWhenNoShapeComponent(self, request, workspace, editor, launcher_platform):
        from .tests.shape_collider import ShapeCollider_InactiveWhenNoShapeComponent as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_Collider_CheckDefaultShapeSettingIsPxMesh(self, request, workspace, editor, launcher_platform):
        from .tests.collider import Collider_CheckDefaultShapeSettingIsPxMesh as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_ShapeCollider_LargeNumberOfShapeCollidersWontCrashEditor(self, request, workspace, editor, launcher_platform):
        from .tests.shape_collider import ShapeCollider_LargeNumberOfShapeCollidersWontCrashEditor as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_Collider_SphereShapeEditing(self, request, workspace, editor, launcher_platform):
        from .tests.collider import Collider_SphereShapeEditing as test_module
        self._run_test(request, workspace, editor, test_module,
                       extra_cmdline_args=["--regset=/Amazon/Preferences/EnablePrefabSystem=true"])

    @revert_physics_config
    def test_Collider_BoxShapeEditing(self, request, workspace, editor, launcher_platform):
        from .tests.collider import Collider_BoxShapeEditing as test_module
        self._run_test(request, workspace, editor, test_module,
                       extra_cmdline_args=["--regset=/Amazon/Preferences/EnablePrefabSystem=true"])

    @revert_physics_config
    def test_Collider_CapsuleShapeEditing(self, request, workspace, editor, launcher_platform):
        from .tests.collider import Collider_CapsuleShapeEditing as test_module
        self._run_test(request, workspace, editor, test_module,
                       extra_cmdline_args=["--regset=/Amazon/Preferences/EnablePrefabSystem=true"])

    def test_ForceRegion_WithNonTriggerColliderWarning(self, request, workspace, editor, launcher_platform):
        from .tests.force_region import ForceRegion_WithNonTriggerColliderWarning as test_module
        # Fixme: expected_lines = ["[Warning] (PhysX Force Region) - Please ensure collider component marked as trigger exists in entity"]
        self._run_test(request, workspace, editor, test_module)

    def test_ForceRegion_WorldSpaceForceOnRigidBodies(self, request, workspace, editor, launcher_platform):
        from .tests.force_region import ForceRegion_WorldSpaceForceOnRigidBodies as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_ForceRegion_PointForceOnRigidBodies(self, request, workspace, editor, launcher_platform):
        from .tests.force_region import ForceRegion_PointForceOnRigidBodies as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_ForceRegion_SphereShapedForce(self, request, workspace, editor, launcher_platform):
        from .tests.force_region import ForceRegion_SphereShapedForce as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_ForceRegion_RotationalOffset(self, request, workspace, editor, launcher_platform):
        from .tests.force_region import ForceRegion_RotationalOffset as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_Material_LibraryClearingAssignsDefault(self, request, workspace, editor, launcher_platform):
        from .tests.material import Material_LibraryClearingAssignsDefault as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_Collider_AddColliderComponent(self, request, workspace, editor, launcher_platform):
        from .tests.collider import Collider_AddColliderComponent as test_module
        self._run_test(request, workspace, editor, test_module)

    @pytest.mark.xfail(
        reason="This will fail due to this issue ATOM-15487.")
    def test_Collider_PxMeshAutoAssignedWhenModifyingRenderMeshComponent(self, request, workspace, editor, launcher_platform):
        from .tests.collider import Collider_PxMeshAutoAssignedWhenModifyingRenderMeshComponent as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_Collider_PxMeshAutoAssignedWhenAddingRenderMeshComponent(self, request, workspace, editor, launcher_platform):
        from .tests.collider import Collider_PxMeshAutoAssignedWhenAddingRenderMeshComponent as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_Collider_MultipleSurfaceSlots(self, request, workspace, editor, launcher_platform):
        from .tests.collider import Collider_MultipleSurfaceSlots as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_Collider_PxMeshNotAutoAssignedWhenNoPhysicsFbx(self, request, workspace, editor, launcher_platform):
        from .tests.collider import Collider_PxMeshNotAutoAssignedWhenNoPhysicsFbx as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_RigidBody_EnablingGravityWorksPoC(self, request, workspace, editor, launcher_platform):
        from .tests.rigid_body import RigidBody_EnablingGravityWorksPoC as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    @fm.file_override('physxsystemconfiguration.setreg','Collider_CollisionGroupsWorkflow.setreg_override',
                      'AutomatedTesting/Registry', search_subdirs=True)
    def test_Collider_CollisionGroupsWorkflow(self, request, workspace, editor, launcher_platform):
        from .tests.collider import Collider_CollisionGroupsWorkflow as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_Collider_ColliderRotationOffset(self, request, workspace, editor, launcher_platform):
        from .tests.collider import Collider_ColliderRotationOffset as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_ForceRegion_ParentChildForcesCombineForces(self, request, workspace, editor, launcher_platform):
        from .tests.force_region import ForceRegion_ParentChildForcesCombineForces as test_module
        self._run_test(request, workspace, editor, test_module)
        
    @revert_physics_config
    def test_ShapeCollider_CanBeAddedWitNoWarnings(self, request, workspace, editor, launcher_platform):
        from .tests.shape_collider import ShapeCollider_CanBeAddedWitNoWarnings as test_module
        self._run_test(request, workspace, editor, test_module)
        
    @revert_physics_config
    def test_Physics_UndoRedoWorksOnEntityWithPhysComponents(self, request, workspace, editor, launcher_platform):
        from .tests import Physics_UndoRedoWorksOnEntityWithPhysComponents as test_module
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