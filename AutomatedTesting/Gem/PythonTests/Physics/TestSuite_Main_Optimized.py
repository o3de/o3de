"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import pytest
import os
import sys
import inspect

from ly_test_tools import LAUNCHERS
from ly_test_tools.o3de.editor_test import EditorSingleTest, EditorSharedTest, EditorParallelTest, EditorTestSuite
from .utils.FileManagement import FileManagement as fm

# Custom test spec, it provides functionality to override files
class EditorSingleTest_WithFileOverrides(EditorSingleTest):
    # Specify here what files to override, [(original, override), ...]
    files_to_override = [()]
    # Base directory of the files (Default path is {ProjectName})
    base_dir = None
    # True will will search sub-directories for the files in base 
    search_subdirs = True

    @classmethod
    def wrap_run(cls, instance, request, workspace, editor, editor_test_results, launcher_platform):
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

        yield # Run Test
        for f in original_file_list:
            fm._restore_file(f, file_list[f])

@pytest.mark.xfail(reason="Optimized tests are experimental, we will enable xfail and monitor them temporarily.")
@pytest.mark.SUITE_main
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomationWithPrefabSystemEnabled(EditorTestSuite):

    @staticmethod
    def get_number_parallel_editors():
        return 16

    class C4982801_PhysXColliderShape_CanBeSelected(EditorSharedTest):
        from .tests.collider import Collider_BoxShapeEditing as test_module

    class C4982800_PhysXColliderShape_CanBeSelected(EditorSharedTest):
        from .tests.collider import Collider_SphereShapeEditing as test_module

    class C4982802_PhysXColliderShape_CanBeSelected(EditorSharedTest):
        from .tests.collider import Collider_CapsuleShapeEditing as test_module

@pytest.mark.xfail(reason="Optimized tests are experimental, we will enable xfail and monitor them temporarily.")
@pytest.mark.SUITE_main
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomation(EditorTestSuite):

    enable_prefab_system = False

    @staticmethod
    def get_number_parallel_editors():
        return 16

    #########################################
    # Non-atomic tests: These need to be run in a single editor because they have custom setup and teardown
    class C4044459_Material_DynamicFriction(EditorSingleTest_WithFileOverrides):
        from .tests.material import Material_DynamicFriction as test_module        
        files_to_override = [
            ('physxsystemconfiguration.setreg', 'Material_DynamicFriction.setreg_override')
        ]
        base_dir = "AutomatedTesting/Registry"

    class C4982593_PhysXCollider_CollisionLayerTest(EditorSingleTest_WithFileOverrides):
        from .tests.collider import Collider_DiffCollisionGroupDiffCollidingLayersNotCollide as test_module
        files_to_override = [
            ('physxsystemconfiguration.setreg', 'Collider_DiffCollisionGroupDiffCollidingLayersNotCollide.setreg_override')
        ]
        base_dir = "AutomatedTesting/Registry"
    #########################################

    class C111111_RigidBody_EnablingGravityWorksUsingNotificationsPoC(EditorSharedTest):
        from .tests.rigid_body import RigidBody_EnablingGravityWorksUsingNotificationsPoC as test_module

    class C5932041_PhysXForceRegion_LocalSpaceForceOnRigidBodies(EditorSharedTest):
        from .tests.force_region import ForceRegion_LocalSpaceForceOnRigidBodies as test_module

    class C15425929_Undo_Redo(EditorSharedTest):
        from .tests import Physics_UndoRedoWorksOnEntityWithPhysComponents as test_module
        
    class C4976243_Collision_SameCollisionGroupDiffCollisionLayers(EditorSharedTest):
        from .tests.collider import Collider_SameCollisionGroupDiffLayersCollide as test_module

    class C14654881_CharacterController_SwitchLevels(EditorSharedTest):
        from .tests.character_controller import CharacterController_SwitchLevels as test_module
 
    class C17411467_AddPhysxRagdollComponent(EditorSharedTest):
        from .tests.ragdoll import Ragdoll_AddPhysxRagdollComponentWorks as test_module
 
    class C12712453_ScriptCanvas_MultipleRaycastNode(EditorSharedTest):
        from .tests.script_canvas import ScriptCanvas_MultipleRaycastNode as test_module

    class C18243586_Joints_HingeLeadFollowerCollide(EditorSharedTest):
        from .tests.joints import Joints_HingeLeadFollowerCollide as test_module

    class C4982803_Enable_PxMesh_Option(EditorSharedTest):
        from .tests.collider import Collider_PxMeshConvexMeshCollides as test_module
    
    class C24308873_CylinderShapeCollider_CollidesWithPhysXTerrain(EditorSharedTest):
        from .tests.shape_collider import ShapeCollider_CylinderShapeCollides as test_module

    class C3510642_Terrain_NotCollideWithTerrain(EditorSharedTest):
        from .tests.terrain import Terrain_NoPhysTerrainComponentNoCollision as test_module

    class C4976195_RigidBodies_InitialLinearVelocity(EditorSharedTest):
        from .tests.rigid_body import RigidBody_InitialLinearVelocity as test_module

    class C4976206_RigidBodies_GravityEnabledActive(EditorSharedTest):
        from .tests.rigid_body import RigidBody_StartGravityEnabledWorks as test_module

    class C4976207_PhysXRigidBodies_KinematicBehavior(EditorSharedTest):
        from .tests.rigid_body import RigidBody_KinematicModeWorks as test_module

    class C5932042_PhysXForceRegion_LinearDamping(EditorSharedTest):
        from .tests.force_region import ForceRegion_LinearDampingForceOnRigidBodies as test_module

    class C5932043_ForceRegion_SimpleDragOnRigidBodies(EditorSharedTest):
        from .tests.force_region import ForceRegion_SimpleDragForceOnRigidBodies as test_module

    class C5959760_PhysXForceRegion_PointForceExertion(EditorSharedTest):
        from .tests.force_region import ForceRegion_CapsuleShapedForce as test_module

    class C5959764_ForceRegion_ForceRegionImpulsesCapsule(EditorSharedTest):
        from .tests.force_region import ForceRegion_ImpulsesCapsuleShapedRigidBody as test_module

    class C5340400_RigidBody_ManualMomentOfInertia(EditorSharedTest):
        from .tests.rigid_body import RigidBody_MomentOfInertiaManualSetting as test_module

    class C4976210_COM_ManualSetting(EditorSharedTest):
        from .tests.rigid_body import RigidBody_COM_ManualSettingWorks as test_module

    class C4976194_RigidBody_PhysXComponentIsValid(EditorSharedTest):
        from .tests.rigid_body import RigidBody_AddRigidBodyComponent as test_module

    class C5932045_ForceRegion_Spline(EditorSharedTest):
        from .tests.force_region import ForceRegion_SplineForceOnRigidBodies as test_module

    class C4982797_Collider_ColliderOffset(EditorSharedTest):
        from .tests.collider import Collider_ColliderPositionOffset as test_module

    class C4976200_RigidBody_AngularDampingObjectRotation(EditorSharedTest):
        from .tests.rigid_body import RigidBody_AngularDampingAffectsRotation as test_module

    class C5689529_Verify_Terrain_RigidBody_Collider_Mesh(EditorSharedTest):
        from .tests import Physics_VerifyColliderRigidBodyMeshAndTerrainWorkTogether as test_module

    class C5959810_ForceRegion_ForceRegionCombinesForces(EditorSharedTest):
        from .tests.force_region import ForceRegion_MultipleForcesInSameComponentCombineForces as test_module

    class C5959765_ForceRegion_AssetGetsImpulsed(EditorSharedTest):
        from .tests.force_region import ForceRegion_ImpulsesPxMeshShapedRigidBody as test_module

    class C6274125_ScriptCanvas_TriggerEvents(EditorSharedTest):
        from .tests.script_canvas import ScriptCanvas_TriggerEvents as test_module
        # needs to be updated to log for unexpected lines
        # expected_lines = test_module.LogLines.expected_lines

    class C6090554_ForceRegion_PointForceNegative(EditorSharedTest):
        from .tests.force_region import ForceRegion_ZeroPointForceDoesNothing as test_module

    class C6090550_ForceRegion_WorldSpaceForceNegative(EditorSharedTest):
        from .tests.force_region import ForceRegion_ZeroWorldSpaceForceDoesNothing as test_module

    class C6090552_ForceRegion_LinearDampingNegative(EditorSharedTest):
        from .tests.force_region import ForceRegion_ZeroLinearDampingDoesNothing as test_module

    class C5968760_ForceRegion_CheckNetForceChange(EditorSharedTest):
        from .tests.force_region import ForceRegion_MovingForceRegionChangesNetForce as test_module

    @pytest.mark.xfail(reason="Ongoing issue in Script Canvas, AZ::Event fail to compile on Script Canvas")
    class C12712452_ScriptCanvas_CollisionEvents(EditorSharedTest):
        from .tests.script_canvas import ScriptCanvas_CollisionEvents as test_module

    class C12868578_ForceRegion_DirectionHasNoAffectOnMagnitude(EditorSharedTest):
        from .tests.force_region import ForceRegion_DirectionHasNoAffectOnTotalForce as test_module

    class C4976204_Verify_Start_Asleep_Condition(EditorSharedTest):
        from .tests.rigid_body import RigidBody_StartAsleepWorks as test_module

    class C6090546_ForceRegion_SliceFileInstantiates(EditorSharedTest):
        from .tests.force_region import ForceRegion_SliceFileInstantiates as test_module

    class C6090551_ForceRegion_LocalSpaceForceNegative(EditorSharedTest):
        from .tests.force_region import ForceRegion_ZeroLocalSpaceForceDoesNothing as test_module

    class C6090553_ForceRegion_SimpleDragForceOnRigidBodies(EditorSharedTest):
        from .tests.force_region import ForceRegion_ZeroSimpleDragForceDoesNothing as test_module

    class C4976209_RigidBody_ComputesCOM(EditorSharedTest):
        from .tests.rigid_body import RigidBody_COM_ComputingWorks as test_module

    class C4976201_RigidBody_MassIsAssigned(EditorSharedTest):
        from .tests.rigid_body import RigidBody_MassDifferentValuesWorks as test_module

    class C12868580_ForceRegion_SplineModifiedTransform(EditorSharedTest):
        from .tests.force_region import ForceRegion_SplineRegionWithModifiedTransform as test_module

    class C12712455_ScriptCanvas_ShapeCastVerification(EditorSharedTest):
        from .tests.script_canvas import ScriptCanvas_ShapeCast as test_module

    class C4976197_RigidBodies_InitialAngularVelocity(EditorSharedTest):
        from .tests.rigid_body import RigidBody_InitialAngularVelocity as test_module

    class C6090555_ForceRegion_SplineFollowOnRigidBodies(EditorSharedTest):
        from .tests.force_region import ForceRegion_ZeroSplineForceDoesNothing as test_module

    class C6131473_StaticSlice_OnDynamicSliceSpawn(EditorSharedTest):
        from .tests import Physics_DynamicSliceWithPhysNotSpawnsStaticSlice as test_module

    class C5959808_ForceRegion_PositionOffset(EditorSharedTest):
        from .tests.force_region import ForceRegion_PositionOffset as test_module

    @pytest.mark.xfail(reason="Something with the CryRenderer disabling is causing this test to fail now.")
    class C13895144_Ragdoll_ChangeLevel(EditorSharedTest):
        from .tests.ragdoll import Ragdoll_LevelSwitchDoesNotCrash as test_module
    
    class C5968759_ForceRegion_ExertsSeveralForcesOnRigidBody(EditorSharedTest):
        from .tests.force_region import ForceRegion_MultipleComponentsCombineForces as test_module

    @pytest.mark.xfail(reason="This test will sometimes fail as the ball will continue to roll before the timeout is reached.")
    class C4976202_RigidBody_StopsWhenBelowKineticThreshold(EditorSharedTest):
        from .tests.rigid_body import RigidBody_SleepWhenBelowKineticThreshold as test_module

    class C13351703_COM_NotIncludeTriggerShapes(EditorSharedTest):
        from .tests.rigid_body import RigidBody_COM_NotIncludesTriggerShapes as test_module

    class C5296614_PhysXMaterial_ColliderShape(EditorSharedTest):
        from .tests.material import Material_NoEffectIfNoColliderShape as test_module

    class C4982595_Collider_TriggerDisablesCollision(EditorSharedTest):
        from .tests.collider import Collider_TriggerPassThrough as test_module

    class C14976307_Gravity_SetGravityWorks(EditorSharedTest):
        from .tests.rigid_body import RigidBody_SetGravityWorks as test_module

    class C4044694_Material_EmptyLibraryUsesDefault(EditorSharedTest):
        from .tests.material import Material_EmptyLibraryUsesDefault as test_module

    class C15845879_ForceRegion_HighLinearDampingForce(EditorSharedTest):
        from .tests.force_region import ForceRegion_NoQuiverOnHighLinearDampingForce as test_module

    class C4976218_RigidBodies_InertiaObjectsNotComputed(EditorSharedTest):
        from .tests.rigid_body import RigidBody_ComputeInertiaWorks as test_module

    class C14902098_ScriptCanvas_PostPhysicsUpdate(EditorSharedTest):
        from .tests.script_canvas import ScriptCanvas_PostPhysicsUpdate as test_module
        # Note: Test needs to be updated to log for unexpected lines
        # unexpected_lines = ["Assert"] + test_module.Lines.unexpected

    class C5959761_ForceRegion_PhysAssetExertsPointForce(EditorSharedTest):
        from .tests.force_region import ForceRegion_PxMeshShapedForce as test_module
        
    # Marking the Test as expected to fail using the xfail decorator due to sporadic failure on Automated Review: SPEC-3146
    # The test still runs, but a failure of the test doesn't result in the test run failing
    @pytest.mark.xfail(reason="Test Sporadically fails with message [ NOT FOUND ] Success: Bar1 : Expected angular velocity")
    class C13352089_RigidBodies_MaxAngularVelocity(EditorSharedTest):
        from .tests.rigid_body import RigidBody_MaxAngularVelocityWorks as test_module

    class C18243584_Joints_HingeSoftLimitsConstrained(EditorSharedTest):
        from .tests.joints import Joints_HingeSoftLimitsConstrained as test_module

    class C18243589_Joints_BallSoftLimitsConstrained(EditorSharedTest):
        from .tests.joints import Joints_BallSoftLimitsConstrained as test_module

    class C18243591_Joints_BallLeadFollowerCollide(EditorSharedTest):
        from .tests.joints import Joints_BallLeadFollowerCollide as test_module

    class C19578018_ShapeColliderWithNoShapeComponent(EditorSharedTest):
        from .tests.shape_collider import ShapeCollider_InactiveWhenNoShapeComponent as test_module

    class C14861500_DefaultSetting_ColliderShape(EditorSharedTest):
        from .tests.collider import Collider_CheckDefaultShapeSettingIsPxMesh as test_module

    class C19723164_ShapeCollider_WontCrashEditor(EditorSharedTest):
        from .tests.shape_collider import ShapeCollider_LargeNumberOfShapeCollidersWontCrashEditor as test_module

    class C12905528_ForceRegion_WithNonTriggerCollider(EditorSharedTest):
        from .tests.force_region import ForceRegion_WithNonTriggerColliderWarning as test_module
        # Fixme: expected_lines = ["[Warning] (PhysX Force Region) - Please ensure collider component marked as trigger exists in entity"]
        
    class C5932040_ForceRegion_CubeExertsWorldForce(EditorSharedTest):
        from .tests.force_region import ForceRegion_WorldSpaceForceOnRigidBodies as test_module
        
    class C5932044_ForceRegion_PointForceOnRigidBody(EditorSharedTest):
        from .tests.force_region import ForceRegion_PointForceOnRigidBodies as test_module
        
    class C5959759_RigidBody_ForceRegionSpherePointForce(EditorSharedTest):
        from .tests.force_region import ForceRegion_SphereShapedForce as test_module
        
    class C5959809_ForceRegion_RotationalOffset(EditorSharedTest):
        from .tests.force_region import ForceRegion_RotationalOffset as test_module
       
    class C15096740_Material_LibraryUpdatedCorrectly(EditorSharedTest):
        from .tests.material import Material_LibraryClearingAssignsDefault as test_module
        
    class C4976236_AddPhysxColliderComponent(EditorSharedTest):
        from .tests.collider import Collider_AddColliderComponent as test_module

    @pytest.mark.xfail(reason="This will fail due to this issue ATOM-15487.")
    class C14861502_PhysXCollider_AssetAutoAssigned(EditorSharedTest):
        from .tests.collider import Collider_PxMeshAutoAssignedWhenModifyingRenderMeshComponent as test_module
        
    class C14861501_PhysXCollider_RenderMeshAutoAssigned(EditorSharedTest):
        from .tests.collider import Collider_PxMeshAutoAssignedWhenAddingRenderMeshComponent as test_module
        
    class C4044695_PhysXCollider_AddMultipleSurfaceFbx(EditorSharedTest):
        from .tests.collider import Collider_MultipleSurfaceSlots as test_module
        
    class C14861504_RenderMeshAsset_WithNoPxAsset(EditorSharedTest):
        from .tests.collider import Collider_PxMeshNotAutoAssignedWhenNoPhysicsFbx as test_module
        
    class C100000_RigidBody_EnablingGravityWorksPoC(EditorSharedTest):
        from .tests.rigid_body import RigidBody_EnablingGravityWorksPoC as test_module

    class C4982798_Collider_ColliderRotationOffset(EditorSharedTest):
        from .tests.collider import Collider_ColliderRotationOffset as test_module

    class C6090547_ForceRegion_ParentChildForceRegions(EditorSharedTest):
        from .tests.force_region import ForceRegion_ParentChildForcesCombineForces as test_module

    class C19578021_ShapeCollider_CanBeAdded(EditorSharedTest):
        from .tests.shape_collider import ShapeCollider_CanBeAddedWitNoWarnings as test_module

    class C15425929_Undo_Redo(EditorSharedTest):
        from .tests import Physics_UndoRedoWorksOnEntityWithPhysComponents as test_module
