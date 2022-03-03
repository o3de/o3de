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

@pytest.mark.SUITE_main
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomation(EditorTestSuite):

    global_extra_cmdline_args = ['-BatchMode', '-autotest_mode',
                                 '--regset=/Amazon/Preferences/EnablePrefabSystem=true']

    @staticmethod
    def get_number_parallel_editors():
        return 16

    class Collider_BoxShapeEditing(EditorSharedTest):
        from .tests.collider import Collider_BoxShapeEditing as test_module

    class Collider_SphereShapeEditing(EditorSharedTest):
        from .tests.collider import Collider_SphereShapeEditing as test_module

    class Collider_CapsuleShapeEditing(EditorSharedTest):
        from .tests.collider import Collider_CapsuleShapeEditing as test_module

@pytest.mark.SUITE_main
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomation(TestAutomationBase):

    def test_RigidBody_EnablingGravityWorksUsingNotificationsPoC(self, request, workspace, editor, launcher_platform):
        from .tests.rigid_body import RigidBody_EnablingGravityWorksUsingNotificationsPoC as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_ForceRegion_LocalSpaceForceOnRigidBodies(self, request, workspace, editor, launcher_platform):
        from .tests.force_region import ForceRegion_LocalSpaceForceOnRigidBodies as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    @fm.file_override('physxsystemconfiguration.setreg','Material_DynamicFriction.setreg_override',
                      'AutomatedTesting/Registry', search_subdirs=True)
    def test_Material_DynamicFriction(self, request, workspace, editor, launcher_platform):
        from .tests.material import Material_DynamicFriction as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_Collider_SameCollisionGroupDiffLayersCollide(self, request, workspace, editor, launcher_platform):
        from .tests.collider import Collider_SameCollisionGroupDiffLayersCollide as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_CharacterController_SwitchLevels(self, request, workspace, editor, launcher_platform):
        from .tests.character_controller import CharacterController_SwitchLevels as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_Ragdoll_AddPhysxRagdollComponentWorks(self, request, workspace, editor, launcher_platform):
        from .tests.ragdoll import Ragdoll_AddPhysxRagdollComponentWorks as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_ScriptCanvas_MultipleRaycastNode(self, request, workspace, editor, launcher_platform):
        from .tests.script_canvas import ScriptCanvas_MultipleRaycastNode as test_module
        # Fixme: This test previously relied on unexpected lines log reading with is now not supported.
        # Now the log reading must be done inside the test, preferably with the Tracer() utility
        #  unexpected_lines = ["Assert"] + test_module.Lines.unexpected
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    @fm.file_override('physxsystemconfiguration.setreg','Collider_DiffCollisionGroupDiffCollidingLayersNotCollide.setreg_override',
                      'AutomatedTesting/Registry', search_subdirs=True)
    def test_Collider_DiffCollisionGroupDiffCollidingLayersNotCollide(self, request, workspace, editor, launcher_platform):
        from .tests.collider import Collider_DiffCollisionGroupDiffCollidingLayersNotCollide as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_Joints_HingeLeadFollowerCollide(self, request, workspace, editor, launcher_platform):
        from .tests.joints import Joints_HingeLeadFollowerCollide as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_Collider_PxMeshConvexMeshCollides(self, request, workspace, editor, launcher_platform):
        from .tests.collider import Collider_PxMeshConvexMeshCollides as test_module
        self._run_test(request, workspace, editor, test_module)
    
    @revert_physics_config    
    def test_ShapeCollider_CylinderShapeCollides(self, request, workspace, editor, launcher_platform):
        from .tests.shape_collider import ShapeCollider_CylinderShapeCollides as test_module
        self._run_test(request, workspace, editor, test_module)

    @revert_physics_config
    def test_C15425929_Undo_Redo(self, request, workspace, editor, launcher_platform):
        from .tests import Physics_UndoRedoWorksOnEntityWithPhysComponents as test_module
        self._run_test(request, workspace, editor, test_module, enable_prefab_system=False)

    @pytest.mark.GROUP_tick
    @pytest.mark.xfail(reason="Test still under development.")
    class Tick_InterpolatedRigidBodyMotionIsSmooth(EditorSharedTest):
        from .tests.tick import Tick_InterpolatedRigidBodyMotionIsSmooth as test_module

    @pytest.mark.GROUP_tick
    @pytest.mark.xfail(reason="Test still under development.")
    class Tick_CharacterGameplayComponentMotionIsSmooth(EditorSharedTest):
        from .tests.tick import Tick_CharacterGameplayComponentMotionIsSmooth as test_module