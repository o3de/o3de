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
    @revert_physics_config
    @fm.file_override('physxsystemconfiguration.setreg','Material_Restitution.setreg_override',
                      'AutomatedTesting/Registry', search_subdirs=True)
    def test_Material_Restitution(self, request, workspace, editor, launcher_platform):
        from .tests.material import Material_Restitution as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_Physics_WorldBodyBusWorksOnEditorComponents(self, request, workspace, editor, launcher_platform):
        from .tests import Physics_WorldBodyBusWorksOnEditorComponents as test_module
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

    class Collider_MultipleSurfaceSlots(EditorBatchedTest):
        from .tests.collider import Collider_MultipleSurfaceSlots as test_module

    @pytest.mark.GROUP_tick
    @pytest.mark.xfail(reason="Test still under development.")
    class Tick_InterpolatedRigidBodyMotionIsSmooth(EditorBatchedTest):
        from .tests.tick import Tick_InterpolatedRigidBodyMotionIsSmooth as test_module

    @pytest.mark.GROUP_tick
    @pytest.mark.xfail(reason="Test still under development.")
    class Tick_CharacterGameplayComponentMotionIsSmooth(EditorBatchedTest):
        from .tests.tick import Tick_CharacterGameplayComponentMotionIsSmooth as test_module

    @pytest.mark.xfail(reason="AssertionError: Couldn't find Asset with path: Objects/SphereBot/r0-b_body.fbx.azmodel")
    class Collider_PxMeshAutoAssignedWhenModifyingRenderMeshComponent(EditorBatchedTest):
        from .tests.collider import Collider_PxMeshAutoAssignedWhenModifyingRenderMeshComponent as test_module

    @pytest.mark.skip(reason="GHI #9301: Test Periodically Fails")
    class Collider_PxMeshNotAutoAssignedWhenNoPhysicsFbx(EditorBatchedTest):
        from .tests.collider import Collider_PxMeshNotAutoAssignedWhenNoPhysicsFbx as test_module
