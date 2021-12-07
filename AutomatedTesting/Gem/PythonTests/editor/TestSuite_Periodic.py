"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
import pytest
import sys

import ly_test_tools.environment.file_system as file_system

sys.path.append(os.path.dirname(os.path.abspath(__file__)) + '/../automatedtesting_shared')
from base import TestAutomationBase


@pytest.fixture
def remove_test_level(request, workspace, project):
    file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", "tmp_level")], True, True)

    def teardown():
        file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", "tmp_level")], True, True)

    request.addfinalizer(teardown)


@pytest.mark.SUITE_periodic
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomation(TestAutomationBase):

    def test_AssetBrowser_TreeNavigation(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import AssetBrowser_TreeNavigation as test_module
        self._run_test(request, workspace, editor, test_module, batch_mode=False, enable_prefab_system=False)

    def test_AssetBrowser_SearchFiltering(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import AssetBrowser_SearchFiltering as test_module
        self._run_test(request, workspace, editor, test_module, batch_mode=False, enable_prefab_system=False)

    def test_AssetPicker_UI_UX(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import AssetPicker_UI_UX as test_module
        self._run_test(request, workspace, editor, test_module, autotest_mode=False, batch_mode=False, enable_prefab_system=False)

    def test_ComponentCRUD_Add_Delete_Components(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import ComponentCRUD_Add_Delete_Components as test_module
        self._run_test(request, workspace, editor, test_module, batch_mode=False, enable_prefab_system=False)

    def test_InputBindings_Add_Remove_Input_Events(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import InputBindings_Add_Remove_Input_Events as test_module
        self._run_test(request, workspace, editor, test_module, batch_mode=False, autotest_mode=False, enable_prefab_system=False)

    def test_Menus_ViewMenuOptions_Work(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import Menus_ViewMenuOptions as test_module
        self._run_test(request, workspace, editor, test_module, batch_mode=False, enable_prefab_system=False)

    @pytest.mark.skip(reason="Times out due to dialogs failing to dismiss: LYN-4208")
    def test_Menus_FileMenuOptions_Work(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import Menus_FileMenuOptions as test_module
        self._run_test(request, workspace, editor, test_module, batch_mode=False, enable_prefab_system=False)
