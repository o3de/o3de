"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
import pytest

import ly_test_tools.environment.file_system as file_system
from ly_test_tools.o3de.editor_test import EditorSingleTest, EditorSharedTest, EditorParallelTest, EditorTestSuite


@pytest.mark.xfail(reason="Optimized tests are experimental, we will enable xfail and monitor them temporarily.")
@pytest.mark.SUITE_main
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomationNoAutoTestMode(EditorTestSuite):

    # Disable -autotest_mode and -BatchMode. Tests cannot run in -BatchMode due to UI interactions, and these tests
    # interact with modal dialogs
    global_extra_cmdline_args = []
    
    enable_prefab_system = False

    class test_BasicEditorWorkflows_LevelEntityComponentCRUD(EditorSingleTest):
        # Custom teardown to remove slice asset created during test
        def teardown(self, request, workspace, editor, editor_test_results, launcher_platform):
            file_system.delete([os.path.join(workspace.paths.engine_root(), "AutomatedTesting", "Levels", "tmp_level")],
                               True, True)
        from .EditorScripts import BasicEditorWorkflows_LevelEntityComponentCRUD as test_module

    @pytest.mark.REQUIRES_gpu
    class test_BasicEditorWorkflows_GPU_LevelEntityComponentCRUD(EditorSingleTest):
        # Disable null renderer
        use_null_renderer = False

        # Custom teardown to remove slice asset created during test
        def teardown(self, request, workspace, editor, editor_test_results, launcher_platform):
            file_system.delete([os.path.join(workspace.paths.engine_root(), "AutomatedTesting", "Levels", "tmp_level")],
                               True, True)
        from .EditorScripts import BasicEditorWorkflows_LevelEntityComponentCRUD as test_module

    class test_InputBindings_Add_Remove_Input_Events(EditorSharedTest):
        from .EditorScripts import InputBindings_Add_Remove_Input_Events as test_module

    class test_AssetPicker_UI_UX(EditorSharedTest):
        from .EditorScripts import AssetPicker_UI_UX as test_module


@pytest.mark.SUITE_main
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomationAutoTestMode(EditorTestSuite):

    # Enable only -autotest_mode for these tests. Tests cannot run in -BatchMode due to UI interactions
    global_extra_cmdline_args = ["-autotest_mode"]

    enable_prefab_system = False

    class test_AssetBrowser_TreeNavigation(EditorSharedTest):
        from .EditorScripts import AssetBrowser_TreeNavigation as test_module

    class test_AssetBrowser_SearchFiltering(EditorSharedTest):
        from .EditorScripts import AssetBrowser_SearchFiltering as test_module

    class test_ComponentCRUD_Add_Delete_Components(EditorSharedTest):
        from .EditorScripts import ComponentCRUD_Add_Delete_Components as test_module

    class test_Menus_ViewMenuOptions_Work(EditorSharedTest):
        from .EditorScripts import Menus_ViewMenuOptions as test_module

    @pytest.mark.skip(reason="Times out due to dialogs failing to dismiss: LYN-4208")
    class test_Menus_FileMenuOptions_Work(EditorSharedTest):
        from .EditorScripts import Menus_FileMenuOptions as test_module

    class test_BasicEditorWorkflows_ExistingLevel_EntityComponentCRUD(EditorSharedTest):
        from .EditorScripts import BasicEditorWorkflows_ExistingLevel_EntityComponentCRUD as test_module
