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
from .FileManagement import FileManagement as fm

# Custom test spec, it provides functionality to override files
class EditorSingleTest_WithFileOverrides(EditorSingleTest):
    # Specify here what files to override, [(original, override), ...]
    files_to_override = [()]
    # Base directory of the files (Default path is {ProjectName})
    base_dir = None
    # True will will search sub-directories for the files in base 
    search_subdirs = False

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
            fm._copy_file(override, file_list[override], original, file_list[override])

        yield # Run Test
        for f in original_file_list:
            fm._restore_file(f, file_list[f])


@pytest.mark.xfail(reason="Optimized tests are experimental, we will enable xfail and monitor them temporarly.")
@pytest.mark.SUITE_main
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomation(EditorTestSuite):

    @staticmethod
    def get_number_parallel_editors():
        return 16

    #########################################
    # Non-atomic tests: These need to be run in a single editor because they have custom setup and teardown
    class C4044459_Material_DynamicFriction(EditorSingleTest_WithFileOverrides):
        from .material import C4044459_Material_DynamicFriction as test_module        
        files_to_override = [
            ('physxsystemconfiguration.setreg', 'C4044459_Material_DynamicFriction.setreg_override')
        ]
        base_dir = "AutomatedTesting/Registry"

    class C4982593_PhysXCollider_CollisionLayerTest(EditorSingleTest_WithFileOverrides):
        from .collider import C4982593_PhysXCollider_CollisionLayerTest as test_module
        files_to_override = [
            ('physxsystemconfiguration.setreg', 'C4982593_PhysXCollider_CollisionLayer.setreg_override')
        ]
        base_dir = "AutomatedTesting/Registry"
    #########################################

    class C111111_RigidBody_EnablingGravityWorksUsingNotificationsPoC(EditorSharedTest):
        from .collider import C111111_RigidBody_EnablingGravityWorksUsingNotificationsPoC as test_module

    class C5932041_PhysXForceRegion_LocalSpaceForceOnRigidBodies(EditorSharedTest):
        from .force_region import C5932041_PhysXForceRegion_LocalSpaceForceOnRigidBodies as test_module

    class C15425929_Undo_Redo(EditorSharedTest):
        from .general import C15425929_Undo_Redo as test_module
        
    class C4976243_Collision_SameCollisionGroupDiffCollisionLayers(EditorSharedTest):
        from .collider import C4976243_Collision_SameCollisionGroupDiffCollisionLayers as test_module

    class C14654881_CharacterController_SwitchLevels(EditorSharedTest):
        from .character_controller import C14654881_CharacterController_SwitchLevels as test_module
 
    class C17411467_AddPhysxRagdollComponent(EditorSharedTest):
        from .ragdoll import C17411467_AddPhysxRagdollComponent as test_module
 
    class C12712453_ScriptCanvas_MultipleRaycastNode(EditorSharedTest):
        from .script_canvas import C12712453_ScriptCanvas_MultipleRaycastNode as test_module

    class C18243586_Joints_HingeLeadFollowerCollide(EditorSharedTest):
        from .joints import C18243586_Joints_HingeLeadFollowerCollide as test_module

    class C4982803_Enable_PxMesh_Option(EditorSharedTest):
        from .collider import C4982803_Enable_PxMesh_Option as test_module
    
    class C24308873_CylinderShapeCollider_CollidesWithPhysXTerrain(EditorSharedTest):
        from .collider import C24308873_CylinderShapeCollider_CollidesWithPhysXTerrain as test_module

    class C3510642_Terrain_NotCollideWithTerrain(EditorSharedTest):
        from .terrain import C3510642_Terrain_NotCollideWithTerrain as test_module

    class C4976195_RigidBodies_InitialLinearVelocity(EditorSharedTest):
        from .rigid_body import C4976195_RigidBodies_InitialLinearVelocity as test_module

    class C4976206_RigidBodies_GravityEnabledActive(EditorSharedTest):
        from .rigid_body import C4976206_RigidBodies_GravityEnabledActive as test_module

    class C4976207_PhysXRigidBodies_KinematicBehavior(EditorSharedTest):
        from .rigid_body import C4976207_PhysXRigidBodies_KinematicBehavior as test_module

    class C5932042_PhysXForceRegion_LinearDamping(EditorSharedTest):
        from .force_region import C5932042_PhysXForceRegion_LinearDamping as test_module

    class C5932043_ForceRegion_SimpleDragOnRigidBodies(EditorSharedTest):
        from .force_region import C5932043_ForceRegion_SimpleDragOnRigidBodies as test_module

    class C5959760_PhysXForceRegion_PointForceExertion(EditorSharedTest):
        from .force_region import C5959760_PhysXForceRegion_PointForceExertion as test_module

    class C5959764_ForceRegion_ForceRegionImpulsesCapsule(EditorSharedTest):
        from .force_region import C5959764_ForceRegion_ForceRegionImpulsesCapsule as test_module

    class C5340400_RigidBody_ManualMomentOfInertia(EditorSharedTest):
        from .rigid_body import C5340400_RigidBody_ManualMomentOfInertia as test_module

    class C4976210_COM_ManualSetting(EditorSharedTest):
        from .rigid_body import C4976210_COM_ManualSetting as test_module

    class C4976194_RigidBody_PhysXComponentIsValid(EditorSharedTest):
        from .rigid_body import C4976194_RigidBody_PhysXComponentIsValid as test_module

    class C5932045_ForceRegion_Spline(EditorSharedTest):
        from .force_region import C5932045_ForceRegion_Spline as test_module

    class C4982797_Collider_ColliderOffset(EditorSharedTest):
        from .collider import C4982797_Collider_ColliderOffset as test_module

    class C4976200_RigidBody_AngularDampingObjectRotation(EditorSharedTest):
        from .rigid_body import C4976200_RigidBody_AngularDampingObjectRotation as test_module

    class C5689529_Verify_Terrain_RigidBody_Collider_Mesh(EditorSharedTest):
        from .general import C5689529_Verify_Terrain_RigidBody_Collider_Mesh as test_module

    class C5959810_ForceRegion_ForceRegionCombinesForces(EditorSharedTest):
        from .force_region import C5959810_ForceRegion_ForceRegionCombinesForces as test_module

    class C5959765_ForceRegion_AssetGetsImpulsed(EditorSharedTest):
        from .force_region import C5959765_ForceRegion_AssetGetsImpulsed as test_module

    class C6274125_ScriptCanvas_TriggerEvents(EditorSharedTest):
        from .script_canvas import C6274125_ScriptCanvas_TriggerEvents as test_module
        # needs to be updated to log for unexpected lines
        # expected_lines = test_module.LogLines.expected_lines

    class C6090554_ForceRegion_PointForceNegative(EditorSharedTest):
        from .force_region import C6090554_ForceRegion_PointForceNegative as test_module

    class C6090550_ForceRegion_WorldSpaceForceNegative(EditorSharedTest):
        from .force_region import C6090550_ForceRegion_WorldSpaceForceNegative as test_module

    class C6090552_ForceRegion_LinearDampingNegative(EditorSharedTest):
        from .force_region import C6090552_ForceRegion_LinearDampingNegative as test_module

    class C5968760_ForceRegion_CheckNetForceChange(EditorSharedTest):
        from .force_region import C5968760_ForceRegion_CheckNetForceChange as test_module

    class C12712452_ScriptCanvas_CollisionEvents(EditorSharedTest):
        from .script_canvas import C12712452_ScriptCanvas_CollisionEvents as test_module

    class C12868578_ForceRegion_DirectionHasNoAffectOnMagnitude(EditorSharedTest):
        from .force_region import C12868578_ForceRegion_DirectionHasNoAffectOnMagnitude as test_module

    class C4976204_Verify_Start_Asleep_Condition(EditorSharedTest):
        from .rigid_body import C4976204_Verify_Start_Asleep_Condition as test_module

    class C6090546_ForceRegion_SliceFileInstantiates(EditorSharedTest):
        from .force_region import C6090546_ForceRegion_SliceFileInstantiates as test_module

    class C6090551_ForceRegion_LocalSpaceForceNegative(EditorSharedTest):
        from .force_region import C6090551_ForceRegion_LocalSpaceForceNegative as test_module

    class C6090553_ForceRegion_SimpleDragForceOnRigidBodies(EditorSharedTest):
        from .force_region import C6090553_ForceRegion_SimpleDragForceOnRigidBodies as test_module

    class C4976209_RigidBody_ComputesCOM(EditorSharedTest):
        from .rigid_body import C4976209_RigidBody_ComputesCOM as test_module

    class C4976201_RigidBody_MassIsAssigned(EditorSharedTest):
        from .rigid_body import C4976201_RigidBody_MassIsAssigned as test_module

    class C12868580_ForceRegion_SplineModifiedTransform(EditorSharedTest):
        from .force_region import C12868580_ForceRegion_SplineModifiedTransform as test_module

    class C12712455_ScriptCanvas_ShapeCastVerification(EditorSharedTest):
        from .script_canvas import C12712455_ScriptCanvas_ShapeCastVerification as test_module

    class C4976197_RigidBodies_InitialAngularVelocity(EditorSharedTest):
        from .rigid_body import C4976197_RigidBodies_InitialAngularVelocity as test_module

    class C6090555_ForceRegion_SplineFollowOnRigidBodies(EditorSharedTest):
        from .force_region import C6090555_ForceRegion_SplineFollowOnRigidBodies as test_module

    class C6131473_StaticSlice_OnDynamicSliceSpawn(EditorSharedTest):
        from .general import C6131473_StaticSlice_OnDynamicSliceSpawn as test_module

    class C5959808_ForceRegion_PositionOffset(EditorSharedTest):
        from .force_region import C5959808_ForceRegion_PositionOffset as test_module

    @pytest.mark.xfail(reason="Something with the CryRenderer disabling is causing this test to fail now.")
    class C13895144_Ragdoll_ChangeLevel(EditorSharedTest):
        from .ragdoll import C13895144_Ragdoll_ChangeLevel as test_module
    
    class C5968759_ForceRegion_ExertsSeveralForcesOnRigidBody(EditorSharedTest):
        from .force_region import C5968759_ForceRegion_ExertsSeveralForcesOnRigidBody as test_module

    @pytest.mark.xfail(reason="This test will sometimes fail as the ball will continue to roll before the timeout is reached.")
    class C4976202_RigidBody_StopsWhenBelowKineticThreshold(EditorSharedTest):
        from .rigid_body import C4976202_RigidBody_StopsWhenBelowKineticThreshold as test_module

    class C13351703_COM_NotIncludeTriggerShapes(EditorSharedTest):
        from .rigid_body import C13351703_COM_NotIncludeTriggerShapes as test_module

    class C5296614_PhysXMaterial_ColliderShape(EditorSharedTest):
        from .material import C5296614_PhysXMaterial_ColliderShape as test_module

    class C4982595_Collider_TriggerDisablesCollision(EditorSharedTest):
        from .collider import C4982595_Collider_TriggerDisablesCollision as test_module

    class C14976307_Gravity_SetGravityWorks(EditorSharedTest):
        from .general import C14976307_Gravity_SetGravityWorks as test_module

    class C4044694_Material_EmptyLibraryUsesDefault(EditorSharedTest):
        from .material import C4044694_Material_EmptyLibraryUsesDefault as test_module

    class C15845879_ForceRegion_HighLinearDampingForce(EditorSharedTest):
        from .force_region import C15845879_ForceRegion_HighLinearDampingForce as test_module

    class C4976218_RigidBodies_InertiaObjectsNotComputed(EditorSharedTest):
        from .rigid_body import C4976218_RigidBodies_InertiaObjectsNotComputed as test_module

    class C14902098_ScriptCanvas_PostPhysicsUpdate(EditorSharedTest):
        from .script_canvas import C14902098_ScriptCanvas_PostPhysicsUpdate as test_module
        # Note: Test needs to be updated to log for unexpected lines
        # unexpected_lines = ["Assert"] + test_module.Lines.unexpected

    class C5959761_ForceRegion_PhysAssetExertsPointForce(EditorSharedTest):
        from .force_region import C5959761_ForceRegion_PhysAssetExertsPointForce as test_module
        
    # Marking the Test as expected to fail using the xfail decorator due to sporadic failure on Automated Review: SPEC-3146
    # The test still runs, but a failure of the test doesn't result in the test run failing
    @pytest.mark.xfail(reason="Test Sporadically fails with message [ NOT FOUND ] Success: Bar1 : Expected angular velocity")
    class C13352089_RigidBodies_MaxAngularVelocity(EditorSharedTest):
        from .rigid_body import C13352089_RigidBodies_MaxAngularVelocity as test_module

    class C18243584_Joints_HingeSoftLimitsConstrained(EditorSharedTest):
        from .joints import C18243584_Joints_HingeSoftLimitsConstrained as test_module

    class C18243589_Joints_BallSoftLimitsConstrained(EditorSharedTest):
        from .joints import C18243589_Joints_BallSoftLimitsConstrained as test_module

    class C18243591_Joints_BallLeadFollowerCollide(EditorSharedTest):
        from .joints import C18243591_Joints_BallLeadFollowerCollide as test_module

    class C19578018_ShapeColliderWithNoShapeComponent(EditorSharedTest):
        from .collider import C19578018_ShapeColliderWithNoShapeComponent as test_module

    class C14861500_DefaultSetting_ColliderShape(EditorSharedTest):
        from .collider import C14861500_DefaultSetting_ColliderShape as test_module

    class C19723164_ShapeCollider_WontCrashEditor(EditorSharedTest):
        from .collider import C19723164_ShapeColliders_WontCrashEditor as test_module

    class C4982800_PhysXColliderShape_CanBeSelected(EditorSharedTest):
        from .collider import C4982800_PhysXColliderShape_CanBeSelected as test_module

    class C4982801_PhysXColliderShape_CanBeSelected(EditorSharedTest):
        from .collider import C4982801_PhysXColliderShape_CanBeSelected as test_module

    class C4982802_PhysXColliderShape_CanBeSelected(EditorSharedTest):
        from .collider import C4982802_PhysXColliderShape_CanBeSelected as test_module
       
    class C12905528_ForceRegion_WithNonTriggerCollider(EditorSharedTest):
        from .force_region import C12905528_ForceRegion_WithNonTriggerCollider as test_module
        # Fixme: expected_lines = ["[Warning] (PhysX Force Region) - Please ensure collider component marked as trigger exists in entity"]
        
    class C5932040_ForceRegion_CubeExertsWorldForce(EditorSharedTest):
        from .force_region import C5932040_ForceRegion_CubeExertsWorldForce as test_module
        
    class C5932044_ForceRegion_PointForceOnRigidBody(EditorSharedTest):
        from .force_region import C5932044_ForceRegion_PointForceOnRigidBody as test_module
        
    class C5959759_RigidBody_ForceRegionSpherePointForce(EditorSharedTest):
        from .force_region import C5959759_RigidBody_ForceRegionSpherePointForce as test_module
        
    class C5959809_ForceRegion_RotationalOffset(EditorSharedTest):
        from .force_region import C5959809_ForceRegion_RotationalOffset as test_module
       
    class C15096740_Material_LibraryUpdatedCorrectly(EditorSharedTest):
        from .material import C15096740_Material_LibraryUpdatedCorrectly as test_module
        
    class C4976236_AddPhysxColliderComponent(EditorSharedTest):
        from .collider import C4976236_AddPhysxColliderComponent as test_module
        

    @pytest.mark.xfail(reason="This will fail due to this issue ATOM-15487.")
    class C14861502_PhysXCollider_AssetAutoAssigned(EditorSharedTest):
        from .collider import C14861502_PhysXCollider_AssetAutoAssigned as test_module
        
    class C14861501_PhysXCollider_RenderMeshAutoAssigned(EditorSharedTest):
        from .collider import C14861501_PhysXCollider_RenderMeshAutoAssigned as test_module
        
    class C4044695_PhysXCollider_AddMultipleSurfaceFbx(EditorSharedTest):
        from .collider import C4044695_PhysXCollider_AddMultipleSurfaceFbx as test_module
        
    class C14861504_RenderMeshAsset_WithNoPxAsset(EditorSharedTest):
        from .collider import C14861504_RenderMeshAsset_WithNoPxAsset as test_module
        
    class C100000_RigidBody_EnablingGravityWorksPoC(EditorSharedTest):
        from .collider import C100000_RigidBody_EnablingGravityWorksPoC as test_module

    class C4982798_Collider_ColliderRotationOffset(EditorSharedTest):
        from .collider import C4982798_Collider_ColliderRotationOffset as test_module

    class C15308217_NoCrash_LevelSwitch(EditorSharedTest):
        from .terrain import C15308217_NoCrash_LevelSwitch as test_module

    class C6090547_ForceRegion_ParentChildForceRegions(EditorSharedTest):
        from .force_region import C6090547_ForceRegion_ParentChildForceRegions as test_module

    class C19578021_ShapeCollider_CanBeAdded(EditorSharedTest):
        from .collider import C19578021_ShapeCollider_CanBeAdded as test_module

    class C15425929_Undo_Redo(EditorSharedTest):
        from .general import C15425929_Undo_Redo as test_module
