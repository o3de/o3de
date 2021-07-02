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


@pytest.mark.SUITE_periodic
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomation(TestAutomationBase):

    @revert_physics_config
    def test_C3510642_Terrain_NotCollideWithTerrain(self, request, workspace, editor, launcher_platform):
        from . import C3510642_Terrain_NotCollideWithTerrain as test_module
        self._run_test(request, workspace, editor, test_module)
        
    @revert_physics_config
    def test_C4976195_RigidBodies_InitialLinearVelocity(self, request, workspace, editor, launcher_platform):
        from . import C4976195_RigidBodies_InitialLinearVelocity as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_C4976206_RigidBodies_GravityEnabledActive(self, request, workspace, editor, launcher_platform):
        from . import C4976206_RigidBodies_GravityEnabledActive as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_C4976207_PhysXRigidBodies_KinematicBehavior(self, request, workspace, editor, launcher_platform):
        from . import C4976207_PhysXRigidBodies_KinematicBehavior as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_C5932042_PhysXForceRegion_LinearDamping(self, request, workspace, editor, launcher_platform):
        from . import C5932042_PhysXForceRegion_LinearDamping as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_C5932043_ForceRegion_SimpleDragOnRigidBodies(self, request, workspace, editor, launcher_platform):
        from . import C5932043_ForceRegion_SimpleDragOnRigidBodies as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_C5959760_PhysXForceRegion_PointForceExertion(self, request, workspace, editor, launcher_platform):
        from . import C5959760_PhysXForceRegion_PointForceExertion as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_C5959764_ForceRegion_ForceRegionImpulsesCapsule(self, request, workspace, editor, launcher_platform):
        from . import C5959764_ForceRegion_ForceRegionImpulsesCapsule as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_C5340400_RigidBody_ManualMomentOfInertia(self, request, workspace, editor, launcher_platform):
        from . import C5340400_RigidBody_ManualMomentOfInertia as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_C4976210_COM_ManualSetting(self, request, workspace, editor, launcher_platform):
        from . import C4976210_COM_ManualSetting as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_C4976194_RigidBody_PhysXComponentIsValid(self, request, workspace, editor, launcher_platform):
        from . import C4976194_RigidBody_PhysXComponentIsValid as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_C5932045_ForceRegion_Spline(self, request, workspace, editor, launcher_platform):
        from . import C5932045_ForceRegion_Spline as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    @fm.file_override('physxsystemconfiguration.setreg','C4044457_Material_RestitutionCombine.setreg_override', 'AutomatedTesting/Registry')
    def test_C4044457_Material_RestitutionCombine(self, request, workspace, editor, launcher_platform):
        from . import C4044457_Material_RestitutionCombine as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    @fm.file_override('physxsystemconfiguration.setreg','C4044456_Material_FrictionCombine.setreg_override', 'AutomatedTesting/Registry')
    def test_C4044456_Material_FrictionCombine(self, request, workspace, editor, launcher_platform):
        from . import C4044456_Material_FrictionCombine as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_C4982797_Collider_ColliderOffset(self, request, workspace, editor, launcher_platform):
        from . import C4982797_Collider_ColliderOffset as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_C4976200_RigidBody_AngularDampingObjectRotation(self, request, workspace, editor, launcher_platform):
        from . import C4976200_RigidBody_AngularDampingObjectRotation as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_C5689529_Verify_Terrain_RigidBody_Collider_Mesh(self, request, workspace, editor, launcher_platform):
        from . import C5689529_Verify_Terrain_RigidBody_Collider_Mesh as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_C5959810_ForceRegion_ForceRegionCombinesForces(self, request, workspace, editor, launcher_platform):
        from . import C5959810_ForceRegion_ForceRegionCombinesForces as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_C5959765_ForceRegion_AssetGetsImpulsed(self, request, workspace, editor, launcher_platform):
        from . import C5959765_ForceRegion_AssetGetsImpulsed as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_C6274125_ScriptCanvas_TriggerEvents(self, request, workspace, editor, launcher_platform):
        from . import C6274125_ScriptCanvas_TriggerEvents as test_module
        # FIXME: expected_lines = test_module.LogLines.expected_lines
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_C6090554_ForceRegion_PointForceNegative(self, request, workspace, editor, launcher_platform):
        from . import C6090554_ForceRegion_PointForceNegative as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_C6090550_ForceRegion_WorldSpaceForceNegative(self, request, workspace, editor, launcher_platform):
        from . import C6090550_ForceRegion_WorldSpaceForceNegative as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_C6090552_ForceRegion_LinearDampingNegative(self, request, workspace, editor, launcher_platform):
        from . import C6090552_ForceRegion_LinearDampingNegative as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_C5968760_ForceRegion_CheckNetForceChange(self, request, workspace, editor, launcher_platform):
        from . import C5968760_ForceRegion_CheckNetForceChange as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_C12712452_ScriptCanvas_CollisionEvents(self, request, workspace, editor, launcher_platform):
        from . import C12712452_ScriptCanvas_CollisionEvents as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_C12868578_ForceRegion_DirectionHasNoAffectOnMagnitude(self, request, workspace, editor, launcher_platform):
        from . import C12868578_ForceRegion_DirectionHasNoAffectOnMagnitude as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_C4976204_Verify_Start_Asleep_Condition(self, request, workspace, editor, launcher_platform):
        from . import C4976204_Verify_Start_Asleep_Condition as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_C6090546_ForceRegion_SliceFileInstantiates(self, request, workspace, editor, launcher_platform):
        from . import C6090546_ForceRegion_SliceFileInstantiates as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_C6090551_ForceRegion_LocalSpaceForceNegative(self, request, workspace, editor, launcher_platform):
        from . import C6090551_ForceRegion_LocalSpaceForceNegative as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_C6090553_ForceRegion_SimpleDragForceOnRigidBodies(self, request, workspace, editor, launcher_platform):
        from . import C6090553_ForceRegion_SimpleDragForceOnRigidBodies as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_C4976209_RigidBody_ComputesCOM(self, request, workspace, editor, launcher_platform):
        from . import C4976209_RigidBody_ComputesCOM as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_C4976201_RigidBody_MassIsAssigned(self, request, workspace, editor, launcher_platform):
        from . import C4976201_RigidBody_MassIsAssigned as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    @fm.file_override('physxsystemconfiguration.setreg','C18981526_Material_RestitutionCombinePriority.setreg_override', 'AutomatedTesting/Registry')
    def test_C18981526_Material_RestitutionCombinePriority(self, request, workspace, editor, launcher_platform):
        from . import C18981526_Material_RestitutionCombinePriority as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_C12868580_ForceRegion_SplineModifiedTransform(self, request, workspace, editor, launcher_platform):
        from . import C12868580_ForceRegion_SplineModifiedTransform as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_C12712455_ScriptCanvas_ShapeCastVerification(self, request, workspace, editor, launcher_platform):
        from . import C12712455_ScriptCanvas_ShapeCastVerification as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_C4976197_RigidBodies_InitialAngularVelocity(self, request, workspace, editor, launcher_platform):
        from . import C4976197_RigidBodies_InitialAngularVelocity as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_C6090555_ForceRegion_SplineFollowOnRigidBodies(self, request, workspace, editor, launcher_platform):
        from . import C6090555_ForceRegion_SplineFollowOnRigidBodies as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_C6131473_StaticSlice_OnDynamicSliceSpawn(self, request, workspace, editor, launcher_platform):
        from . import C6131473_StaticSlice_OnDynamicSliceSpawn as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_C5959808_ForceRegion_PositionOffset(self, request, workspace, editor, launcher_platform):
        from . import C5959808_ForceRegion_PositionOffset as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    @fm.file_override('physxsystemconfiguration.setreg','C18977601_Material_FrictionCombinePriority.setreg_override', 'AutomatedTesting/Registry')
    def test_C18977601_Material_FrictionCombinePriority(self, request, workspace, editor, launcher_platform):
        from . import C18977601_Material_FrictionCombinePriority as test_module
        self._run_test(request, workspace, editor, test_module)

    @pytest.mark.xfail(
        reason="Something with the CryRenderer disabling is causing this test to fail now.")
    @revert_physics_config
    def test_C13895144_Ragdoll_ChangeLevel(self, request, workspace, editor, launcher_platform):
        from . import C13895144_Ragdoll_ChangeLevel as test_module
        self._run_test(request, workspace, editor, test_module)
    
    @revert_physics_config
    def test_C5968759_ForceRegion_ExertsSeveralForcesOnRigidBody(self, request, workspace, editor, launcher_platform):
        from . import C5968759_ForceRegion_ExertsSeveralForcesOnRigidBody as test_module
        self._run_test(request, workspace, editor, test_module)

    # Marking the test as an expected failure due to sporadic failure on Automated Review: LYN-2580
    # The test still runs, but a failure of the test doesn't result in the test run failing
    @pytest.mark.xfail(
        reason="This test needs new physics asset with multiple materials to be more stable.")
    @revert_physics_config
    @fm.file_override('physxsystemconfiguration.setreg','C4044697_Material_PerfaceMaterialValidation.setreg_override', 'AutomatedTesting/Registry')
    def test_C4044697_Material_PerfaceMaterialValidation(self, request, workspace, editor, launcher_platform):
        from . import C4044697_Material_PerfaceMaterialValidation as test_module
        self._run_test(request, workspace, editor, test_module)

    @pytest.mark.xfail(
        reason="This test will sometimes fail as the ball will continue to roll before the timeout is reached.")
    @revert_physics_config
    def test_C4976202_RigidBody_StopsWhenBelowKineticThreshold(self, request, workspace, editor, launcher_platform):
        from . import C4976202_RigidBody_StopsWhenBelowKineticThreshold as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_C13351703_COM_NotIncludeTriggerShapes(self, request, workspace, editor, launcher_platform):
        from . import C13351703_COM_NotIncludeTriggerShapes as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_C5296614_PhysXMaterial_ColliderShape(self, request, workspace, editor, launcher_platform):
        from . import C5296614_PhysXMaterial_ColliderShape as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_C4982595_Collider_TriggerDisablesCollision(self, request, workspace, editor, launcher_platform):
        from . import C4982595_Collider_TriggerDisablesCollision as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_C14976307_Gravity_SetGravityWorks(self, request, workspace, editor, launcher_platform):
        from . import C14976307_Gravity_SetGravityWorks as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    @fm.file_override('physxsystemconfiguration.setreg','C15556261_PhysXMaterials_CharacterControllerMaterialAssignment.setreg_override', 'AutomatedTesting/Registry')
    def test_C15556261_PhysXMaterials_CharacterControllerMaterialAssignment(self, request, workspace, editor, launcher_platform):
        from . import C15556261_PhysXMaterials_CharacterControllerMaterialAssignment as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_C4044694_Material_EmptyLibraryUsesDefault(self, request, workspace, editor, launcher_platform):
        from . import C4044694_Material_EmptyLibraryUsesDefault as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_C15845879_ForceRegion_HighLinearDampingForce(self, request, workspace, editor, launcher_platform):
        from . import C15845879_ForceRegion_HighLinearDampingForce as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_C4976218_RigidBodies_InertiaObjectsNotComputed(self, request, workspace, editor, launcher_platform):
        from . import C4976218_RigidBodies_InertiaObjectsNotComputed as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_C14902098_ScriptCanvas_PostPhysicsUpdate(self, request, workspace, editor, launcher_platform):
        from . import C14902098_ScriptCanvas_PostPhysicsUpdate as test_module
        # Fixme: unexpected_lines = ["Assert"] + test_module.Lines.unexpected
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    @fm.file_override('physxsystemconfiguration.setreg','C4976245_PhysXCollider_CollisionLayerTest.setreg_override', 'AutomatedTesting/Registry')
    def test_C4976245_PhysXCollider_CollisionLayerTest(self, request, workspace, editor, launcher_platform):
        from . import C4976245_PhysXCollider_CollisionLayerTest as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    @fm.file_override('physxsystemconfiguration.setreg','C4976244_Collider_SameGroupSameLayerCollision.setreg_override', 'AutomatedTesting/Registry')
    def test_C4976244_Collider_SameGroupSameLayerCollision(self, request, workspace, editor, launcher_platform):
        from . import C4976244_Collider_SameGroupSameLayerCollision as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    @fm.file_override('physxdefaultsceneconfiguration.setreg','C14195074_ScriptCanvas_PostUpdateEvent.setreg_override', 'AutomatedTesting/Registry')
    def test_C14195074_ScriptCanvas_PostUpdateEvent(self, request, workspace, editor, launcher_platform):
        from . import C14195074_ScriptCanvas_PostUpdateEvent as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    @fm.file_override('physxsystemconfiguration.setreg','C4044461_Material_Restitution.setreg_override', 'AutomatedTesting/Registry')
    def test_C4044461_Material_Restitution(self, request, workspace, editor, launcher_platform):
        from . import C4044461_Material_Restitution as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    @fm.file_override('physxdefaultsceneconfiguration.setreg','C14902097_ScriptCanvas_PreUpdateEvent.setreg_override', 'AutomatedTesting/Registry')
    def test_C14902097_ScriptCanvas_PreUpdateEvent(self, request, workspace, editor, launcher_platform):
        from . import C14902097_ScriptCanvas_PreUpdateEvent as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_C5959761_ForceRegion_PhysAssetExertsPointForce(self, request, workspace, editor, launcher_platform):
        from . import C5959761_ForceRegion_PhysAssetExertsPointForce as test_module
        self._run_test(request, workspace, editor, test_module)

    # Marking the Test  as expected to fail using the xfail decorator due to sporadic failure on Automated Review: SPEC-3146
    # The test still runs, but a failure of the test doesn't result in the test run failing
    @pytest.mark.xfail(reason="Test Sporadically fails with message [ NOT FOUND ] Success: Bar1 : Expected angular velocity")
    @revert_physics_config
    def test_C13352089_RigidBodies_MaxAngularVelocity(self, request, workspace, editor, launcher_platform):
        from . import C13352089_RigidBodies_MaxAngularVelocity as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_C18243584_Joints_HingeSoftLimitsConstrained(self, request, workspace, editor, launcher_platform):
        from . import C18243584_Joints_HingeSoftLimitsConstrained as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_C18243589_Joints_BallSoftLimitsConstrained(self, request, workspace, editor, launcher_platform):
        from . import C18243589_Joints_BallSoftLimitsConstrained as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_C18243591_Joints_BallLeadFollowerCollide(self, request, workspace, editor, launcher_platform):
        from . import C18243591_Joints_BallLeadFollowerCollide as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    @fm.file_override('physxsystemconfiguration.setreg','C4976227_Collider_NewGroup.setreg_override', 'AutomatedTesting/Registry')
    def test_C4976227_Collider_NewGroup(self, request, workspace, editor, launcher_platform):
        from . import C4976227_Collider_NewGroup as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_C19578018_ShapeColliderWithNoShapeComponent(self, request, workspace, editor, launcher_platform):
        from . import C19578018_ShapeColliderWithNoShapeComponent as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_C14861500_DefaultSetting_ColliderShape(self, request, workspace, editor, launcher_platform):
        from . import C14861500_DefaultSetting_ColliderShape as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_C19723164_ShapeCollider_WontCrashEditor(self, request, workspace, editor, launcher_platform):
        from . import C19723164_ShapeColliders_WontCrashEditor as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_C4982800_PhysXColliderShape_CanBeSelected(self, request, workspace, editor, launcher_platform):
        from . import C4982800_PhysXColliderShape_CanBeSelected as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_C4982801_PhysXColliderShape_CanBeSelected(self, request, workspace, editor, launcher_platform):
        from . import C4982801_PhysXColliderShape_CanBeSelected as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_C4982802_PhysXColliderShape_CanBeSelected(self, request, workspace, editor, launcher_platform):
        from . import C4982802_PhysXColliderShape_CanBeSelected as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_C12905528_ForceRegion_WithNonTriggerCollider(self, request, workspace, editor, launcher_platform):
        from . import C12905528_ForceRegion_WithNonTriggerCollider as test_module
        # Fixme: expected_lines = ["[Warning] (PhysX Force Region) - Please ensure collider component marked as trigger exists in entity"]
        self._run_test(request, workspace, editor, test_module)

    def test_C5932040_ForceRegion_CubeExertsWorldForce(self, request, workspace, editor, launcher_platform):
        from . import C5932040_ForceRegion_CubeExertsWorldForce as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_C5932044_ForceRegion_PointForceOnRigidBody(self, request, workspace, editor, launcher_platform):
        from . import C5932044_ForceRegion_PointForceOnRigidBody as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_C5959759_RigidBody_ForceRegionSpherePointForce(self, request, workspace, editor, launcher_platform):
        from . import C5959759_RigidBody_ForceRegionSpherePointForce as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_C5959809_ForceRegion_RotationalOffset(self, request, workspace, editor, launcher_platform):
        from . import C5959809_ForceRegion_RotationalOffset as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_C15096740_Material_LibraryUpdatedCorrectly(self, request, workspace, editor, launcher_platform):
        from . import C15096740_Material_LibraryUpdatedCorrectly as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_C4976236_AddPhysxColliderComponent(self, request, workspace, editor, launcher_platform):
        from . import C4976236_AddPhysxColliderComponent as test_module
        self._run_test(request, workspace, editor, test_module)

    @pytest.mark.xfail(
        reason="This will fail due to this issue ATOM-15487.")
    def test_C14861502_PhysXCollider_AssetAutoAssigned(self, request, workspace, editor, launcher_platform):
        from . import C14861502_PhysXCollider_AssetAutoAssigned as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_C14861501_PhysXCollider_RenderMeshAutoAssigned(self, request, workspace, editor, launcher_platform):
        from . import C14861501_PhysXCollider_RenderMeshAutoAssigned as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_C4044695_PhysXCollider_AddMultipleSurfaceFbx(self, request, workspace, editor, launcher_platform):
        from . import C4044695_PhysXCollider_AddMultipleSurfaceFbx as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_C14861504_RenderMeshAsset_WithNoPxAsset(self, request, workspace, editor, launcher_platform):
        from . import C14861504_RenderMeshAsset_WithNoPxAsset as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_C100000_RigidBody_EnablingGravityWorksPoC(self, request, workspace, editor, launcher_platform):
        from . import C100000_RigidBody_EnablingGravityWorksPoC as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    @fm.file_override('physxsystemconfiguration.setreg','C3510644_Collider_CollisionGroups.setreg_override', 'AutomatedTesting/Registry')
    def test_C3510644_Collider_CollisionGroups(self, request, workspace, editor, launcher_platform):
        from . import C3510644_Collider_CollisionGroups as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_C4982798_Collider_ColliderRotationOffset(self, request, workspace, editor, launcher_platform):
        from . import C4982798_Collider_ColliderRotationOffset as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_C15308217_NoCrash_LevelSwitch(self, request, workspace, editor, launcher_platform):
        from . import C15308217_NoCrash_LevelSwitch as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_C6090547_ForceRegion_ParentChildForceRegions(self, request, workspace, editor, launcher_platform):
        from . import C6090547_ForceRegion_ParentChildForceRegions as test_module
        self._run_test(request, workspace, editor, test_module)
        
    @revert_physics_config
    def test_C19578021_ShapeCollider_CanBeAdded(self, request, workspace, editor, launcher_platform):
        from . import C19578021_ShapeCollider_CanBeAdded as test_module
        self._run_test(request, workspace, editor, test_module)
        
    @revert_physics_config
    def test_C15425929_Undo_Redo(self, request, workspace, editor, launcher_platform):
        from . import C15425929_Undo_Redo as test_module
        self._run_test(request, workspace, editor, test_module)
