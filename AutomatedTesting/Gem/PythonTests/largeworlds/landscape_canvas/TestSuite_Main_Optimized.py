"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.
SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
import pytest

import ly_test_tools.environment.file_system as file_system
import ly_test_tools._internal.pytest_plugin as internal_plugin
from ly_test_tools.o3de.editor_test import EditorSingleTest, EditorSharedTest, EditorParallelTest, EditorTestSuite


@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomation(EditorTestSuite):

    class test_LandscapeCanvas_AreaNodes_DependentComponentsAdded(EditorSharedTest):
        from .EditorScripts import AreaNodes_DependentComponentsAdded as test_module

    class test_LandscapeCanvas_AreaNodes_EntityCreatedOnNodeAdd(EditorSharedTest):
        from .EditorScripts import AreaNodes_EntityCreatedOnNodeAdd as test_module

    class test_LandscapeCanvas_AreaNodes_EntityRemovedOnNodeDelete(EditorSharedTest):
        from .EditorScripts import AreaNodes_EntityRemovedOnNodeDelete as test_module

    class test_LandscapeCanvas_Component_AddedRemoved(EditorSharedTest):
        from .EditorScripts import Component_AddedRemoved as test_module

    class test_LandscapeCanvas_ComponentUpdates_UpdateGraph(EditorSharedTest):
        from .EditorScripts import ComponentUpdates_UpdateGraph as test_module

    class test_LandscapeCanvas_Edit_DisabledNodeDuplication(EditorSharedTest):
        from .EditorScripts import Edit_DisabledNodeDuplication as test_module

    class test_LandscapeCanvas_Edit_UndoNodeDelete_PrefabEntity(EditorSharedTest):
        from .EditorScripts import Edit_UndoNodeDelete_PrefabEntity as test_module

    class test_LandscapeCanvas_GradientMixer_NodeConstruction(EditorSharedTest):
        from .EditorScripts import GradientMixer_NodeConstruction as test_module

    class test_LandscapeCanvas_GradientModifierNodes_EntityCreatedOnNodeAdd(EditorSharedTest):
        from .EditorScripts import GradientModifierNodes_EntityCreatedOnNodeAdd as test_module

    class test_LandscapeCanvas_GradientModifierNodes_EntityRemovedOnNodeDelete(EditorSharedTest):
        from .EditorScripts import GradientModifierNodes_EntityRemovedOnNodeDelete as test_module

    class test_LandscapeCanvas_GradientNodes_DependentComponentsAdded(EditorSharedTest):
        from .EditorScripts import GradientNodes_DependentComponentsAdded as test_module

    class test_LandscapeCanvas_GradientNodes_EntityCreatedOnNodeAdd(EditorSharedTest):
        from .EditorScripts import GradientNodes_EntityCreatedOnNodeAdd as test_module

    class test_LandscapeCanvas_GradientNodes_EntityRemovedOnNodeDelete(EditorSharedTest):
        from .EditorScripts import GradientNodes_EntityRemovedOnNodeDelete as test_module

    @pytest.mark.xfail(reason="https://github.com/o3de/o3de/issues/2201")
    class test_LandscapeCanvas_GraphClosed_OnEntityDelete(EditorSharedTest):
        from .EditorScripts import GraphClosed_OnEntityDelete as test_module

    class test_LandscapeCanvas_GraphClosed_OnLevelChange(EditorSharedTest):
        from .EditorScripts import GraphClosed_OnLevelChange as test_module

    class test_LandscapeCanvas_GraphClosed_TabbedGraphClosesIndependently(EditorSharedTest):
        from .EditorScripts import GraphClosed_TabbedGraph as test_module

    @pytest.mark.skipif("debug" == os.path.basename(internal_plugin.build_directory),
                        reason="https://github.com/o3de/o3de/issues/4872")
    class test_LandscapeCanvas_GraphUpdates_UpdateComponents(EditorSharedTest):
        from .EditorScripts import GraphUpdates_UpdateComponents as test_module

    class test_LandscapeCanvas_LayerBlender_NodeConstruction(EditorSharedTest):
        from .EditorScripts import LayerBlender_NodeConstruction as test_module

    class test_LandscapeCanvas_LayerExtenderNodes_ComponentEntitySync(EditorSharedTest):
        from .EditorScripts import LayerExtenderNodes_ComponentEntitySync as test_module

    class test_LandscapeCanvas_NewGraph_CreatedSuccessfully(EditorSharedTest):
        from .EditorScripts import NewGraph_CreatedSuccessfully as test_module

    class test_LandscapeCanvas_Prefab_CreateInstantiate(EditorSharedTest):
        from .EditorScripts import Prefab_CreateInstantiate as test_module

    class test_LandscapeCanvas_ShapeNodes_EntityCreatedOnNodeAdd(EditorSharedTest):
        from .EditorScripts import ShapeNodes_EntityCreatedOnNodeAdd as test_module

    class test_LandscapeCanvas_ShapeNodes_EntityRemovedOnNodeDelete(EditorSharedTest):
        from .EditorScripts import ShapeNodes_EntityRemovedOnNodeDelete as test_module

    class test_LandscapeCanvas_SlotConnections_UpdateComponentReferences(EditorSharedTest):
        from .EditorScripts import SlotConnections_UpdateComponentReferences as test_module
