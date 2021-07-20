"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

"""

# This suite consists of all test cases that are under development and have not been verified yet.
# Once they are verified, please move them to TestSuite_Active.py

import pytest
import os
import sys

from .FileManagement import FileManagement as fm

sys.path.append(os.path.dirname(os.path.abspath(__file__)) + '/../automatedtesting_shared')

@pytest.mark.parametrize("spec", ["all"])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.system
class TestAutomation(TestAutomationBase):
    @fm.file_revert("ragdollbones.physmaterial",
                    r"AutomatedTesting\Levels\Physics\C4925582_Material_AddModifyDeleteOnRagdollBones")
    def test_C4925582_Material_AddModifyDeleteOnRagdollBones(self, request, workspace, editor):
        from . import C4925582_Material_AddModifyDeleteOnRagdollBones as test_module

        expected_lines = []
        unexpected_lines = ["Assert"]
        self._run_test(request, workspace, editor, test_module, expected_lines, unexpected_lines)

    def test_C4925580_Material_RagdollBonesMaterial(self, request, workspace, editor):
        from . import C4925580_Material_RagdollBonesMaterial as test_module

        expected_lines = []
        unexpected_lines = ["Assert"]
        self._run_test(request, workspace, editor, test_module, expected_lines, unexpected_lines)

    @fm.file_revert("c15308221_material_componentsinsyncwithlibrary.physmaterial",
                    r"AutomatedTesting\Levels\Physics\C15308221_Material_ComponentsInSyncWithLibrary")
    def test_C15308221_Material_ComponentsInSyncWithLibrary(self, request, workspace, editor):
        from . import C15308221_Material_ComponentsInSyncWithLibrary as test_module

        expected_lines = []
        unexpected_lines = ["Assert"]
        self._run_test(request, workspace, editor, test_module, expected_lines, unexpected_lines)

    # BUG: LY-107723")
    def test_C14976308_ScriptCanvas_SetKinematicTargetTransform(self, request, workspace, editor):
        from . import C14976308_ScriptCanvas_SetKinematicTargetTransform as test_module

        expected_lines = []
        unexpected_lines = ["Assert"]
        self._run_test(request, workspace, editor, test_module, expected_lines, unexpected_lines)

    # Failing, PhysXTerrain
    @fm.file_revert("c4925579_material_addmodifydeleteonterrain.physmaterial",
                    r"AutomatedTesting\Levels\Physics\C4925579_Material_AddModifyDeleteOnTerrain")
    def test_C4925579_Material_AddModifyDeleteOnTerrain(self, request, workspace, editor):
        from . import C4925579_Material_AddModifyDeleteOnTerrain as test_module

        expected_lines = []
        unexpected_lines = ["Assert"]
        self._run_test(request, workspace, editor, test_module, expected_lines, unexpected_lines)

    # Failing, PhysXTerrain
    def test_C13508019_Terrain_TerrainTexturePainterWorks(self, request, workspace, editor):
        from . import C13508019_Terrain_TerrainTexturePainterWorks as test_module

        expected_lines = []
        unexpected_lines = ["Assert"]
        self._run_test(request, workspace, editor, test_module, expected_lines, unexpected_lines)

    # Failing, PhysXTerrain
    def test_C4925577_Materials_MaterialAssignedToTerrain(self, request, workspace, editor):
        from . import C4925577_Materials_MaterialAssignedToTerrain as test_module

        expected_lines = []
        unexpected_lines = ["Assert"]
        self._run_test(request, workspace, editor, test_module, expected_lines, unexpected_lines)

    # Failing, PhysXTerrain
    def test_C15096735_Materials_DefaultLibraryConsistency(self, request, workspace, editor):
        from . import C15096735_Materials_DefaultLibraryConsistency as test_module

        expected_lines = []
        unexpected_lines = ["Assert"]
        self._run_test(request, workspace, editor, test_module, expected_lines, unexpected_lines)

    # Failing, PhysXTerrain
    @fm.file_revert("all_ones_1.physmaterial", r"AutomatedTesting\Levels\Physics\C15096737_Materials_DefaultMaterialLibraryChanges")
    @fm.file_override("default.physxconfiguration", "C15096737_Materials_DefaultMaterialLibraryChanges.physxconfiguration", "AutomatedTesting")
    def test_C15096737_Materials_DefaultMaterialLibraryChanges(self, request, workspace, editor):
        from . import C15096737_Materials_DefaultMaterialLibraryChanges as test_module
        expected_lines = []
        unexpected_lines = ["Assert"]
        self._run_test(request, workspace, editor, test_module, expected_lines, unexpected_lines)

    def test_C4976242_Collision_SameCollisionlayerSameCollisiongroup(self, request, workspace, editor):
        from . import C4976242_Collision_SameCollisionlayerSameCollisiongroup as test_module

        expected_lines = []
        unexpected_lines = ["Assert"]
        self._run_test(request, workspace, editor, test_module, expected_lines, unexpected_lines)

    def test_C15096732_Material_DefaultLibraryUpdatedAcrossLevels(self, request, workspace, editor):
        @fm.file_override("default.physxconfiguration",
                          "Material_DefaultLibraryUpdatedAcrossLevels_before.physxconfiguration", "AutomatedTesting",
                          search_subdirs=True)
        def test_levels_before(self, request, workspace, editor):
            from . import C15096732_Material_DefaultLibraryUpdatedAcrossLevels_before as test_module_0
            expected_lines = []
            unexpected_lines = ["Assert"]
            self._run_test(request, workspace, editor, test_module_0, expected_lines, unexpected_lines)

        # File override replaces the previous physxconfiguration file with another where the only difference is the default material library
        @fm.file_override("default.physxconfiguration",
                          "Material_DefaultLibraryUpdatedAcrossLevels_after.physxconfiguration", "AutomatedTesting",
                          search_subdirs=True)
        def test_levels_after(self, request, workspace, editor):
            from . import C15096732_Material_DefaultLibraryUpdatedAcrossLevels_after as test_module_1
            expected_lines = []
            unexpected_lines = ["Assert"]
            self._run_test(request, workspace, editor, test_module_1, expected_lines, unexpected_lines)

        test_levels_before(self, request, workspace, editor)
        test_levels_after(self, request, workspace, editor)

    def test_C14654882_Ragdoll_ragdollAPTest(self, request, workspace, editor):
        from . import C14654882_Ragdoll_ragdollAPTest as test_module

        expected_lines = []
        unexpected_lines = test_module.UnexpectedLines.lines
        self._run_test(request, workspace, editor, test_module, expected_lines, unexpected_lines)

    @fm.file_override("default.physxconfiguration", "C12712454_ScriptCanvas_OverlapNodeVerification.physxconfiguration")
    def test_C12712454_ScriptCanvas_OverlapNodeVerification(self, request, workspace, editor):
        from . import C12712454_ScriptCanvas_OverlapNodeVerification as test_module

        expected_lines = []
        unexpected_lines = ["Assert"]
        self._run_test(request, workspace, editor, test_module, expected_lines, unexpected_lines)

    def test_C4044460_Material_StaticFriction(self, request, workspace, editor):
        from . import C4044460_Material_StaticFriction as test_module

        expected_lines = []
        unexpected_lines = ["Assert"]
        self._run_test(request, workspace, editor, test_module, expected_lines, unexpected_lines)
        
    
        
    @fm.file_revert("c4888315_material_addmodifydeleteoncollider.physmaterial",
                    r"AutomatedTesting\Levels\Physics\C4888315_Material_AddModifyDeleteOnCollider")
    def test_C4888315_Material_AddModifyDeleteOnCollider(self, request, workspace, editor):
        from . import C4888315_Material_AddModifyDeleteOnCollider as test_module

        expected_lines = []
        unexpected_lines = ["Assert"]

        self._run_test(request, workspace, editor, test_module, expected_lines, unexpected_lines)

    @fm.file_revert("c15563573_material_addmodifydeleteoncharactercontroller.physmaterial",
                    r"AutomatedTesting\Levels\Physics\C15563573_Material_AddModifyDeleteOnCharacterController")
    def test_C15563573_Material_AddModifyDeleteOnCharacterController(self, request, workspace, editor):
        from . import C15563573_Material_AddModifyDeleteOnCharacterController as test_module

        expected_lines = []
        unexpected_lines = ["Assert"]
        self._run_test(request, workspace, editor, test_module, expected_lines, unexpected_lines)

    @fm.file_revert("c4888315_material_addmodifydeleteoncollider.physmaterial",
                    r"AutomatedTesting\Levels\Physics\C4888315_Material_AddModifyDeleteOnCollider")
    def test_C4888315_Material_AddModifyDeleteOnCollider(self, request, workspace, editor):
        from . import C4888315_Material_AddModifyDeleteOnCollider as test_module

        expected_lines = []
        unexpected_lines = ["Assert"]

        self._run_test(request, workspace, editor, test_module, expected_lines, unexpected_lines)
        
    

    @fm.file_revert("c15563573_material_addmodifydeleteoncharactercontroller.physmaterial",
                    r"AutomatedTesting\Levels\Physics\C15563573_Material_AddModifyDeleteOnCharacterController")
    def test_C15563573_Material_AddModifyDeleteOnCharacterController(self, request, workspace, editor):
        from . import C15563573_Material_AddModifyDeleteOnCharacterController as test_module

        expected_lines = []
        unexpected_lines = ["Assert"]
        self._run_test(request, workspace, editor, test_module, expected_lines, unexpected_lines)

    @fm.file_revert("c4044455_material_librarychangesinstantly.physmaterial",
                    r"AutomatedTesting\Levels\Physics\C4044455_Material_LibraryChangesInstantly")
    def test_C4044455_Material_libraryChangesInstantly(self, request, workspace, editor):
        from . import C4044455_Material_libraryChangesInstantly as test_module

        expected_lines = []
        unexpected_lines = ["Assert"]
        self._run_test(request, workspace, editor, test_module, expected_lines, unexpected_lines)


    @fm.file_revert("C15425935_Material_LibraryUpdatedAcrossLevels.physmaterial",
                    r"AutomatedTesting\Levels\Physics\C15425935_Material_LibraryUpdatedAcrossLevels")
    def test_C15425935_Material_LibraryUpdatedAcrossLevels(self, request, workspace, editor):
        from . import C15425935_Material_LibraryUpdatedAcrossLevels as test_module

        expected_lines = []
        unexpected_lines = ["Assert"]
        self._run_test(request, workspace, editor, test_module, expected_lines, unexpected_lines)
    
    def test_C4976199_RigidBodies_LinearDampingObjectMotion(self, request, workspace, editor):
        from . import C4976199_RigidBodies_LinearDampingObjectMotion as test_module

        expected_lines = []
        unexpected_lines = ["Assert"]
        self._run_test(request, workspace, editor, test_module, expected_lines, unexpected_lines)

    def test_C5689518_PhysXTerrain_CollidesWithPhysXTerrain(self, request, workspace, editor):
        from . import C5689518_PhysXTerrain_CollidesWithPhysXTerrain as test_module

        expected_lines = []
        unexpected_lines = ["Assert"]
        self._run_test(request, workspace, editor, test_module, expected_lines, unexpected_lines)

    def test_C24308873_CylinderShapeCollider_CollidesWithPhysXTerrain(self, request, workspace, editor):
        from . import C24308873_CylinderShapeCollider_CollidesWithPhysXTerrain as test_module

        expected_lines = []
        unexpected_lines = ["Assert"]
        self._run_test(request, workspace, editor, test_module, expected_lines, unexpected_lines)

    def test_C29032500_EditorComponents_WorldBodyBusWorks(self, request, workspace, editor):
        from . import C29032500_EditorComponents_WorldBodyBusWorks as test_module

        expected_lines = []
        unexpected_lines = ["Assert"]
        self._run_test(request, workspace, editor, test_module, expected_lines, unexpected_lines)

    def test_C14861498_ConfirmError_NoPxMesh(self, request, workspace, editor, launcher_platform):
        from . import C14861498_ConfirmError_NoPxMesh as test_module

        expected_lines = []
        unexpected_lines = ["Assert"]
        self._run_test(request, workspace, editor, test_module, expected_lines, unexpected_lines)

    def test_C5959763_ForceRegion_ForceRegionImpulsesCube(self, request, workspace, editor, launcher_platform):
        from . import C5959763_ForceRegion_ForceRegionImpulsesCube as test_module

        expected_lines = []
        unexpected_lines = ["Assert"]
        self._run_test(request, workspace, editor, test_module, expected_lines, unexpected_lines)

    def test_C5689531_Warning_TerrainSliceTerrainComponent(self, request, workspace, editor, launcher_platform):
        from . import C5689531_Warning_TerrainSliceTerrainComponent as test_module
        expected_lines = []
        unexpected_lines = ["Assert"]
        self._run_test(request, workspace, editor, test_module, expected_lines, unexpected_lines)

    def test_C5689522_Physxterrain_AddPhysxterrainNoEditorCrash(self, request, workspace, editor, launcher_platform):
        from . import C5689522_Physxterrain_AddPhysxterrainNoEditorCrash as test_module

        expected_lines = []
        unexpected_lines = ["Assert"]
        self._run_test(request, workspace, editor, test_module, expected_lines, unexpected_lines)

    def test_C5689524_MultipleTerrains_CheckWarningInConsole(self, request, workspace, editor, launcher_platform):
        from . import C5689524_MultipleTerrains_CheckWarningInConsole as test_module

        expected_lines = []
        unexpected_lines = ["Assert"]
        self._run_test(request, workspace, editor, test_module, expected_lines, unexpected_lines)

    def test_C5689528_Terrain_MultipleTerrainComponents(self, request, workspace, editor, launcher_platform):
        from . import C5689528_Terrain_MultipleTerrainComponents as test_module
        expected_lines = []
        unexpected_lines = ["Assert"]
        self._run_test(request, workspace, editor, test_module, expected_lines, unexpected_lines)

    def test_C5689528_Terrain_MultipleTerrainComponents(self, request, workspace, editor, launcher_platform):
        from . import C5689528_Terrain_MultipleTerrainComponents as test_module

        expected_lines = []
        unexpected_lines = ["Assert"]
        self._run_test(request, workspace, editor, test_module, expected_lines, unexpected_lines)

    def test_C6321601_Force_HighValuesDirectionAxes(self, request, workspace, editor, launcher_platform):
        from . import C6321601_Force_HighValuesDirectionAxes as test_module

        expected_lines = []
        unexpected_lines = ["Assert"]
        self._run_test(request, workspace, editor, test_module, expected_lines, unexpected_lines)

    def test_C6032082_Terrain_MultipleResolutionsValid(self, request, workspace, editor, launcher_platform):
        from . import C6032082_Terrain_MultipleResolutionsValid as test_module

        expected_lines = []
        unexpected_lines = ["Assert"]
        self._run_test(request, workspace, editor, test_module, expected_lines, unexpected_lines)

    def test_C12905527_ForceRegion_MagnitudeDeviation(self, request, workspace, editor, launcher_platform):
        from . import C12905527_ForceRegion_MagnitudeDeviation as test_module

        expected_lines = []
        unexpected_lines = ["Assert"]
        self._run_test(request, workspace, editor, test_module, expected_lines, unexpected_lines)

    # Removed from active suite to meet 60 minutes limit in AR job
    def test_C18243580_Joints_Fixed2BodiesConstrained(self, request, workspace, editor, launcher_platform):
        from . import C18243580_Joints_Fixed2BodiesConstrained as test_module

        expected_lines = []
        unexpected_lines = ["Assert"]
        self._run_test(request, workspace, editor, test_module, expected_lines, unexpected_lines)

    # Removed from active suite to meet 60 minutes limit in AR job
    def test_C18243583_Joints_Hinge2BodiesConstrained(self, request, workspace, editor, launcher_platform):
        from . import C18243583_Joints_Hinge2BodiesConstrained as test_module

        expected_lines = []
        unexpected_lines = ["Assert"]
        self._run_test(request, workspace, editor, test_module, expected_lines, unexpected_lines)

    # Removed from active suite to meet 60 minutes limit in AR job
    def test_C18243588_Joints_Ball2BodiesConstrained(self, request, workspace, editor, launcher_platform):
        from . import C18243588_Joints_Ball2BodiesConstrained as test_module

        expected_lines = []
        unexpected_lines = ["Assert"]
        self._run_test(request, workspace, editor, test_module, expected_lines, unexpected_lines)

    # Removed from active suite to meet 60 minutes limit in AR job
    def test_C18243581_Joints_FixedBreakable(self, request, workspace, editor, launcher_platform):
        from . import C18243581_Joints_FixedBreakable as test_module

        expected_lines = []
        unexpected_lines = ["Assert"]
        self._run_test(request, workspace, editor, test_module, expected_lines, unexpected_lines)

    # Removed from active suite to meet 60 minutes limit in AR job
    def test_C18243587_Joints_HingeBreakable(self, request, workspace, editor, launcher_platform):
        from . import C18243587_Joints_HingeBreakable as test_module

        expected_lines = []
        unexpected_lines = ["Assert"]
        self._run_test(request, workspace, editor, test_module, expected_lines, unexpected_lines)

    # Removed from active suite to meet 60 minutes limit in AR job
    def test_C18243592_Joints_BallBreakable(self, request, workspace, editor, launcher_platform):
        from . import C18243592_Joints_BallBreakable as test_module

        expected_lines = []
        unexpected_lines = ["Assert"]
        self._run_test(request, workspace, editor, test_module, expected_lines, unexpected_lines)

    # Removed from active suite to meet 60 minutes limit in AR job
    def test_C18243585_Joints_HingeNoLimitsConstrained(self, request, workspace, editor, launcher_platform):
        from . import C18243585_Joints_HingeNoLimitsConstrained as test_module

        expected_lines = []
        unexpected_lines = ["Assert"]
        self._run_test(request, workspace, editor, test_module, expected_lines, unexpected_lines)

    # Removed from active suite to meet 60 minutes limit in AR job
    def test_C18243590_Joints_BallNoLimitsConstrained(self, request, workspace, editor, launcher_platform):
        from . import C18243590_Joints_BallNoLimitsConstrained as test_module

        expected_lines = []
        unexpected_lines = ["Assert"]
        self._run_test(request, workspace, editor, test_module, expected_lines, unexpected_lines)

    # Removed from active suite to meet 60 minutes limit in AR job
    def test_C18243582_Joints_FixedLeadFollowerCollide(self, request, workspace, editor, launcher_platform):
        from . import C18243582_Joints_FixedLeadFollowerCollide as test_module

        expected_lines = []
        unexpected_lines = ["Assert"]
        self._run_test(request, workspace, editor, test_module, expected_lines, unexpected_lines)

    # Removed from active suite to meet 60 minutes limit in AR job
    def test_C18243593_Joints_GlobalFrameConstrained(self, request, workspace, editor, launcher_platform):
        from . import C18243593_Joints_GlobalFrameConstrained as test_module

        expected_lines = []
        unexpected_lines = ["Assert"]
        self._run_test(request, workspace, editor, test_module, expected_lines, unexpected_lines)
