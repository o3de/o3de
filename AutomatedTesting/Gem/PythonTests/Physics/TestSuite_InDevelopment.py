"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

"""

# This suite consists of all test cases that are under development and have not been verified yet.
# Once they are verified, please move them to TestSuite_Active.py

import pytest
import os
import sys

from .utils.FileManagement import FileManagement as fm

sys.path.append(os.path.dirname(os.path.abspath(__file__)) + '/../automatedtesting_shared')

from base import TestAutomationBase

@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
class TestAutomation(TestAutomationBase):
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

    def test_PhysX_Collider_Component_CRUD(self, request, workspace, editor, launcher_platform):
        from .tests.EntityComponentTests import PhysX_Collider_Component_CRUD as test_module
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
