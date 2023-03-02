"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

"""

# This suite consists of all test cases that are under development and have not been verified yet.

import pytest
import os
import sys

from .utils.FileManagement import FileManagement as fm
from ly_test_tools.o3de.editor_test import EditorSingleTest, EditorBatchedTest, EditorTestSuite

sys.path.append(os.path.dirname(os.path.abspath(__file__)) + '/../automatedtesting_shared')

from base import TestAutomationBase

revert_physics_config = fm.file_revert_list(['physxdebugconfiguration.setreg', 'physxdefaultsceneconfiguration.setreg', 'physxsystemconfiguration.setreg'], 'AutomatedTesting/Registry')

@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
class TestAutomation(TestAutomationBase):
    # Marking the test as an expected failure due to sporadic failure on Automated Review: LYN-2580
    # The test still runs, but a failure of the test doesn't result in the test run failing
    @revert_physics_config
    @fm.file_override('physxsystemconfiguration.setreg', 'Material_PerFaceMaterialGetsCorrectMaterial.setreg_override',
                      'AutomatedTesting/Registry', search_subdirs=True)
    def test_Material_PerFaceMaterialGetsCorrectMaterial(self, request, workspace, editor, launcher_platform):
        from .tests.material import Material_PerFaceMaterialGetsCorrectMaterial as test_module
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
    @fm.file_override('physxsystemconfiguration.setreg','Material_RestitutionCombinePriorityOrder.setreg_override',
                      'AutomatedTesting/Registry', search_subdirs=True)
    def test_Material_RestitutionCombinePriorityOrder(self, request, workspace, editor, launcher_platform):
        from .tests.material import Material_RestitutionCombinePriorityOrder as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    @fm.file_override('physxsystemconfiguration.setreg','Material_FrictionCombinePriorityOrder.setreg_override',
                      'AutomatedTesting/Registry', search_subdirs=True)
    def test_Material_FrictionCombinePriorityOrder(self, request, workspace, editor, launcher_platform):
        from .tests.material import Material_FrictionCombinePriorityOrder as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    @fm.file_override('physxsystemconfiguration.setreg','Material_Restitution.setreg_override',
                      'AutomatedTesting/Registry', search_subdirs=True)
    def test_Material_Restitution(self, request, workspace, editor, launcher_platform):
        from .tests.material import Material_Restitution as test_module
        self._run_test(request, workspace, editor, test_module)

    @fm.file_revert("ragdollbones.physmaterial",
                    r"AutomatedTesting\Levels\Physics\Material_LibraryCrudOperationsReflectOnRagdollBones")
    def test_Material_LibraryCrudOperationsReflectOnRagdollBones(self, request, workspace, editor, launcher_platform):
        from .tests.material import Material_LibraryCrudOperationsReflectOnRagdollBones as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_Material_RagdollBones(self, request, workspace, editor, launcher_platform):
        from .tests.material import Material_RagdollBones as test_module
        self._run_test(request, workspace, editor, test_module)

    @fm.file_revert("c15308221_material_componentsinsyncwithlibrary.physmaterial",
                    r"AutomatedTesting\Levels\Physics\Material_ComponentsInSyncWithLibrary")
    def test_Material_ComponentsInSyncWithLibrary(self, request, workspace, editor, launcher_platform):
        from .tests.material import Material_ComponentsInSyncWithLibrary as test_module
        self._run_test(request, workspace, editor, test_module)

    # BUG: LY-107723")
    def test_ScriptCanvas_SetKinematicTargetTransform(self, request, workspace, editor, launcher_platform):
        from .tests.script_canvas import ScriptCanvas_SetKinematicTargetTransform as test_module
        self._run_test(request, workspace, editor, test_module)

    # Failing, PhysXTerrain
    @fm.file_revert("c4925579_material_addmodifydeleteonterrain.physmaterial",
                    r"AutomatedTesting\Levels\Physics\Material_LibraryCrudOperationsReflectOnTerrain")
    def test_Material_LibraryCrudOperationsReflectOnTerrain(self, request, workspace, editor, launcher_platform):
        from .tests.material import Material_LibraryCrudOperationsReflectOnTerrain as test_module
        self._run_test(request, workspace, editor, test_module)

    # Failing, PhysXTerrain
    def test_Terrain_TerrainTexturePainterWorks(self, request, workspace, editor, launcher_platform):
        from .tests.terrain import Terrain_TerrainTexturePainterWorks as test_module
        self._run_test(request, workspace, editor, test_module)

    # Failing, PhysXTerrain
    def test_Material_CanBeAssignedToTerrain(self, request, workspace, editor, launcher_platform):
        from .tests.material import Material_CanBeAssignedToTerrain as test_module
        self._run_test(request, workspace, editor, test_module)

    # Failing, PhysXTerrain
    def test_Material_DefaultLibraryConsistentOnAllFeatures(self, request, workspace, editor, launcher_platform):
        from .tests.material import Material_DefaultLibraryConsistentOnAllFeatures as test_module
        self._run_test(request, workspace, editor, test_module)

    # Failing, PhysXTerrain
    @fm.file_revert("all_ones_1.physmaterial", r"AutomatedTesting\Levels\Physics\Material_DefaultMaterialLibraryChangesWork")
    @fm.file_override("default.physxconfiguration", "Material_DefaultMaterialLibraryChangesWork.physxconfiguration", "AutomatedTesting")
    def test_Material_DefaultMaterialLibraryChangesWork(self, request, workspace, editor, launcher_platform):
        from .tests.material import Material_DefaultMaterialLibraryChangesWork as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_Collider_SameCollisionGroupSameLayerCollide(self, request, workspace, editor, launcher_platform):
        from .tests.collider import Collider_SameCollisionGroupSameLayerCollide as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_Ragdoll_OldRagdollSerializationNoErrors(self, request, workspace, editor, launcher_platform):
        from .tests.ragdoll import Ragdoll_OldRagdollSerializationNoErrors as test_module
        self._run_test(request, workspace, editor, test_module)

    @fm.file_override("default.physxconfiguration", "ScriptCanvas_OverlapNode.physxconfiguration")
    def test_ScriptCanvas_OverlapNode(self, request, workspace, editor, launcher_platform):
        from .tests.script_canvas import ScriptCanvas_OverlapNode as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_Material_StaticFriction(self, request, workspace, editor, launcher_platform):
        from .tests.material import Material_StaticFriction as test_module
        self._run_test(request, workspace, editor, test_module)
        
    @fm.file_revert("c4888315_material_addmodifydeleteoncollider.physmaterial",
                    r"AutomatedTesting\Levels\Physics\Material_LibraryCrudOperationsReflectOnCollider")
    def test_Material_LibraryCrudOperationsReflectOnCollider(self, request, workspace, editor, launcher_platform):
        from .tests.material import Material_LibraryCrudOperationsReflectOnCollider as test_module
        self._run_test(request, workspace, editor, test_module)

    @fm.file_revert("c15563573_material_addmodifydeleteoncharactercontroller.physmaterial",
                    r"AutomatedTesting\Levels\Physics\Material_LibraryCrudOperationsReflectOnCharacterController")
    def test_Material_LibraryCrudOperationsReflectOnCharacterController(self, request, workspace, editor, launcher_platform):
        from .tests.material import Material_LibraryCrudOperationsReflectOnCharacterController as test_module
        self._run_test(request, workspace, editor, test_module)

    @fm.file_revert("c4044455_material_librarychangesinstantly.physmaterial",
                    r"AutomatedTesting\Levels\Physics\C4044455_Material_LibraryChangesInstantly")
    def test_Material_LibraryChangesReflectInstantly(self, request, workspace, editor, launcher_platform):
        from .tests.material import Material_LibraryChangesReflectInstantly as test_module
        self._run_test(request, workspace, editor, test_module)

    @fm.file_revert("Material_LibraryUpdatedAcrossLevels.physmaterial",
                    r"AutomatedTesting\Levels\Physics\Material_LibraryUpdatedAcrossLevels")
    def test_Material_LibraryUpdatedAcrossLevels(self, request, workspace, editor, launcher_platform):
        from .tests.material import Material_LibraryUpdatedAcrossLevels as test_module
        self._run_test(request, workspace, editor, test_module)
    
    def test_RigidBody_LinearDampingAffectsMotion(self, request, workspace, editor, launcher_platform):
        from .tests.rigid_body import RigidBody_LinearDampingAffectsMotion as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_Terrain_CollisionAgainstRigidBody(self, request, workspace, editor, launcher_platform):
        from .tests.terrain import Terrain_CollisionAgainstRigidBody as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_Physics_WorldBodyBusWorksOnEditorComponents(self, request, workspace, editor, launcher_platform):
        from .tests import Physics_WorldBodyBusWorksOnEditorComponents as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_Collider_PxMeshErrorIfNoMesh(self, request, workspace, editor, launcher_platform):
        from .tests.collider import Collider_PxMeshErrorIfNoMesh as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_ForceRegion_ImpulsesBoxShapedRigidBody(self, request, workspace, editor, launcher_platform):
        from .tests.force_region import ForceRegion_ImpulsesBoxShapedRigidBody as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_Terrain_AddPhysTerrainComponent(self, request, workspace, editor, launcher_platform):
        from .tests.terrain import Terrain_AddPhysTerrainComponent as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_Terrain_CanAddMultipleTerrainComponents(self, request, workspace, editor, launcher_platform):
        from .tests.terrain import Terrain_CanAddMultipleTerrainComponents as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_Terrain_MultipleTerrainComponentsWarning(self, request, workspace, editor, launcher_platform):
        from .tests.terrain import Terrain_MultipleTerrainComponentsWarning as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_ForceRegion_HighValuesDirectionAxesWorkWithNoError(self, request, workspace, editor, launcher_platform):
        from .tests.force_region import ForceRegion_HighValuesDirectionAxesWorkWithNoError as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_Terrain_MultipleResolutionsValid(self, request, workspace, editor, launcher_platform):
        from .tests.terrain import Terrain_MultipleResolutionsValid as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_ForceRegion_SmallMagnitudeDeviationOnLargeForces(self, request, workspace, editor, launcher_platform):
        from .tests.force_region import ForceRegion_SmallMagnitudeDeviationOnLargeForces as test_module
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
    def wrap_run(cls, instance, request, workspace, editor_test_results):
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

    class Collider_MultipleSurfaceSlots(EditorBatchedTest):
        from .tests.collider import Collider_MultipleSurfaceSlots as test_module

    class Material_LibraryClearingAssignsDefault(EditorBatchedTest):
        from .tests.material import Material_LibraryClearingAssignsDefault as test_module

    class Physics_UndoRedoWorksOnEntityWithPhysComponents(EditorBatchedTest):
        from .tests import Physics_UndoRedoWorksOnEntityWithPhysComponents as test_module

    @pytest.mark.GROUP_tick
    @pytest.mark.xfail(reason="Test still under development.")
    class Tick_InterpolatedRigidBodyMotionIsSmooth(EditorBatchedTest):
        from .tests.tick import Tick_InterpolatedRigidBodyMotionIsSmooth as test_module

    @pytest.mark.GROUP_tick
    @pytest.mark.xfail(reason="Test still under development.")
    class Tick_CharacterGameplayComponentMotionIsSmooth(EditorBatchedTest):
        from .tests.tick import Tick_CharacterGameplayComponentMotionIsSmooth as test_module

    @pytest.mark.xfail(reason="AssertionError: Couldn't find Asset with path: Objects/SphereBot/r0-b_body.azmodel")
    class Collider_PxMeshAutoAssignedWhenModifyingRenderMeshComponent(EditorBatchedTest):
        from .tests.collider import Collider_PxMeshAutoAssignedWhenModifyingRenderMeshComponent as test_module

    @pytest.mark.skip(reason="GHI #9301: Test Periodically Fails")
    class Collider_PxMeshNotAutoAssignedWhenNoPhysicsFbx(EditorBatchedTest):
        from .tests.collider import Collider_PxMeshNotAutoAssignedWhenNoPhysicsFbx as test_module

    @pytest.mark.skip(reason="GHI #9364: Test Periodically Fails")
    class ForceRegion_LinearDampingForceOnRigidBodies(EditorBatchedTest):
        from .tests.force_region import ForceRegion_LinearDampingForceOnRigidBodies as test_module

    @pytest.mark.xfail(
        reason="This test will sometimes fail as the ball will continue to roll before the timeout is reached.")
    class RigidBody_SleepWhenBelowKineticThreshold(EditorBatchedTest):
        from .tests.rigid_body import RigidBody_SleepWhenBelowKineticThreshold as test_module

    @pytest.mark.xfail(reason="GHI #9579: Test periodically fails")
    class Collider_TriggerPassThrough(EditorBatchedTest):
        from .tests.collider import Collider_TriggerPassThrough as test_module

    @pytest.mark.skip(reason="GHI #9365: Test periodically fails")
    class ForceRegion_NoQuiverOnHighLinearDampingForce(EditorBatchedTest):
        from .tests.force_region import ForceRegion_NoQuiverOnHighLinearDampingForce as test_module

    @pytest.mark.xfail(reason="GHI #9565: Test periodically fails")
    class RigidBody_ComputeInertiaWorks(EditorBatchedTest):
        from .tests.rigid_body import RigidBody_ComputeInertiaWorks as test_module

    # Marking the Test as expected to fail using the xfail decorator due to sporadic failure on Automated Review: SPEC-3146
    # The test still runs, but a failure of the test doesn't result in the test run failing
    @pytest.mark.xfail(
        reason="Test Sporadically fails with message [ NOT FOUND ] Success: Bar1 : Expected angular velocity")
    class RigidBody_MaxAngularVelocityWorks(EditorBatchedTest):
        from .tests.rigid_body import RigidBody_MaxAngularVelocityWorks as test_module

    @pytest.mark.xfail(reason="GHI #9582: Test periodically fails")
    class ForceRegion_WorldSpaceForceOnRigidBodies(EditorBatchedTest):
        from .tests.force_region import ForceRegion_WorldSpaceForceOnRigidBodies as test_module

    @pytest.mark.xfail(reason="GHI #9566: Test periodically fails")
    class ForceRegion_PointForceOnRigidBodies(EditorBatchedTest):
        from .tests.force_region import ForceRegion_PointForceOnRigidBodies as test_module

    @pytest.mark.xfail(reason="GHI #9368: Test Sporadically Fails")
    class Collider_ColliderRotationOffset(EditorBatchedTest):
        from .tests.collider import Collider_ColliderRotationOffset as test_module
