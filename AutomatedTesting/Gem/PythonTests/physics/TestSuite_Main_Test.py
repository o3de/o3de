"""
Copyright (c) Contributors to the Open 3D Engine Project

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


@pytest.mark.SUITE_main
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomation(EditorTestSuite):
    
    class C4044459_Material_DynamicFriction(EditorSingleTest_WithFileOverrides):
        from . import C4044459_Material_DynamicFriction as test_module        
        files_to_override = [
            ('physxsystemconfiguration.setreg', 'C4044459_Material_DynamicFriction.setreg_override')
        ]
        base_dir = "AutomatedTesting/Registry"

    class C4982593_PhysXCollider_CollisionLayerTest(EditorSingleTest_WithFileOverrides):
        from . import C4982593_PhysXCollider_CollisionLayerTest as test_module
        files_to_override = [
            ('physxsystemconfiguration.setreg', 'C4982593_PhysXCollider_CollisionLayer.setreg_override')
        ]
        base_dir = "AutomatedTesting/Registry"

    class C111111_RigidBody_EnablingGravityWorksUsingNotificationsPoC(EditorSharedTest):
        from . import C111111_RigidBody_EnablingGravityWorksUsingNotificationsPoC as test_module

    class C5932041_PhysXForceRegion_LocalSpaceForceOnRigidBodies(EditorSharedTest):
        from . import C5932041_PhysXForceRegion_LocalSpaceForceOnRigidBodies as test_module

    class C15425929_Undo_Redo(EditorSharedTest):
        from . import C15425929_Undo_Redo as test_module
        
    class C4976243_Collision_SameCollisionGroupDiffCollisionLayers(EditorSharedTest):
        from . import C4976243_Collision_SameCollisionGroupDiffCollisionLayers as test_module

    class C14654881_CharacterController_SwitchLevels(EditorSharedTest):
        from . import C14654881_CharacterController_SwitchLevels as test_module
 
    class C17411467_AddPhysxRagdollComponent(EditorSharedTest):
        from . import C17411467_AddPhysxRagdollComponent as test_module
 
    class C12712453_ScriptCanvas_MultipleRaycastNode(EditorSharedTest):
        from . import C12712453_ScriptCanvas_MultipleRaycastNode as test_module

    class C18243586_Joints_HingeLeadFollowerCollide(EditorSharedTest):
        from . import C18243586_Joints_HingeLeadFollowerCollide as test_module

    class C4982803_Enable_PxMesh_Option(EditorSharedTest):
        from . import C4982803_Enable_PxMesh_Option as test_module
    
    class C24308873_CylinderShapeCollider_CollidesWithPhysXTerrain(EditorSharedTest):
        from . import C24308873_CylinderShapeCollider_CollidesWithPhysXTerrain as test_module
    