"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
import pytest
import sys

import ly_test_tools.environment.file_system as file_system

sys.path.append(os.path.dirname(os.path.abspath(__file__)) + '/../../automatedtesting_shared')
from base import TestAutomationBase


@pytest.fixture
def remove_test_slice(request, workspace, project):
    file_system.delete([os.path.join(workspace.paths.engine_root(), project, "slices", "TestSlice.slice")], True, True)

    def teardown():
        file_system.delete([os.path.join(workspace.paths.engine_root(), project, "slices", "TestSlice.slice")], True,
                           True)

    request.addfinalizer(teardown)


@pytest.mark.SUITE_periodic
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomation(TestAutomationBase):

    def test_LandscapeCanvas_AreaNodes_DependentComponentsAdded(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import AreaNodes_DependentComponentsAdded as test_module
        self._run_test(request, workspace, editor, test_module, enable_prefab_system=False)

    def test_LandscapeCanvas_AreaNodes_EntityCreatedOnNodeAdd(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import AreaNodes_EntityCreatedOnNodeAdd as test_module
        self._run_test(request, workspace, editor, test_module, enable_prefab_system=False)

    def test_LandscapeCanvas_AreaNodes_EntityRemovedOnNodeDelete(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import AreaNodes_EntityRemovedOnNodeDelete as test_module
        self._run_test(request, workspace, editor, test_module, enable_prefab_system=False)

    def test_LandscapeCanvas_LayerExtenderNodes_ComponentEntitySync(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import LayerExtenderNodes_ComponentEntitySync as test_module
        self._run_test(request, workspace, editor, test_module, enable_prefab_system=False)

    def test_LandscapeCanvas_Edit_DisabledNodeDuplication(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import Edit_DisabledNodeDuplication as test_module
        self._run_test(request, workspace, editor, test_module, enable_prefab_system=False)

    def test_LandscapeCanvas_Edit_UndoNodeDelete_SliceEntity(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import Edit_UndoNodeDelete_SliceEntity as test_module
        self._run_test(request, workspace, editor, test_module, enable_prefab_system=False)

    def test_LandscapeCanvas_NewGraph_CreatedSuccessfully(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import NewGraph_CreatedSuccessfully as test_module
        self._run_test(request, workspace, editor, test_module, enable_prefab_system=False)

    def test_LandscapeCanvas_Component_AddedRemoved(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import Component_AddedRemoved as test_module
        self._run_test(request, workspace, editor, test_module, enable_prefab_system=False)

    def test_LandscapeCanvas_GraphClosed_OnLevelChange(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import GraphClosed_OnLevelChange as test_module
        self._run_test(request, workspace, editor, test_module, enable_prefab_system=False)

    @pytest.mark.xfail(reason="https://github.com/o3de/o3de/issues/2201")
    def test_LandscapeCanvas_GraphClosed_OnEntityDelete(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import GraphClosed_OnEntityDelete as test_module
        self._run_test(request, workspace, editor, test_module, enable_prefab_system=False)

    def test_LandscapeCanvas_GraphClosed_TabbedGraphClosesIndependently(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import GraphClosed_TabbedGraph as test_module
        self._run_test(request, workspace, editor, test_module, enable_prefab_system=False)

    def test_LandscapeCanvas_Slice_CreateInstantiate(self, request, workspace, editor, remove_test_slice, launcher_platform):
        from .EditorScripts import Slice_CreateInstantiate as test_module
        self._run_test(request, workspace, editor, test_module, enable_prefab_system=False)

    def test_LandscapeCanvas_GradientModifierNodes_EntityCreatedOnNodeAdd(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import GradientModifierNodes_EntityCreatedOnNodeAdd as test_module
        self._run_test(request, workspace, editor, test_module, enable_prefab_system=False)

    def test_LandscapeCanvas_GradientModifierNodes_EntityRemovedOnNodeDelete(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import GradientModifierNodes_EntityRemovedOnNodeDelete as test_module
        self._run_test(request, workspace, editor, test_module, enable_prefab_system=False)

    def test_LandscapeCanvas_GradientNodes_DependentComponentsAdded(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import GradientNodes_DependentComponentsAdded as test_module
        self._run_test(request, workspace, editor, test_module, enable_prefab_system=False)

    def test_LandscapeCanvas_GradientNodes_EntityCreatedOnNodeAdd(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import GradientNodes_EntityCreatedOnNodeAdd as test_module
        self._run_test(request, workspace, editor, test_module, enable_prefab_system=False)

    def test_LandscapeCanvas_GradientNodes_EntityRemovedOnNodeDelete(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import GradientNodes_EntityRemovedOnNodeDelete as test_module
        self._run_test(request, workspace, editor, test_module, enable_prefab_system=False)

    def test_LandscapeCanvas_GraphUpdates_UpdateComponents(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import GraphUpdates_UpdateComponents as test_module
        self._run_test(request, workspace, editor, test_module, enable_prefab_system=False)

    def test_LandscapeCanvas_ComponentUpdates_UpdateGraph(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import ComponentUpdates_UpdateGraph as test_module
        self._run_test(request, workspace, editor, test_module, enable_prefab_system=False)

    def test_LandscapeCanvas_LayerBlender_NodeConstruction(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import LayerBlender_NodeConstruction as test_module
        self._run_test(request, workspace, editor, test_module, enable_prefab_system=False)

    def test_LandscapeCanvas_ShapeNodes_EntityCreatedOnNodeAdd(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import ShapeNodes_EntityCreatedOnNodeAdd as test_module
        self._run_test(request, workspace, editor, test_module, enable_prefab_system=False)

    def test_LandscapeCanvas_ShapeNodes_EntityRemovedOnNodeDelete(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import ShapeNodes_EntityRemovedOnNodeDelete as test_module
        self._run_test(request, workspace, editor, test_module, enable_prefab_system=False)
