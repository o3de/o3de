"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.
SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import pytest

from ly_test_tools.o3de.editor_test import EditorSingleTest, EditorBatchedTest, EditorParallelTest, EditorTestSuite


@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomation(EditorTestSuite):

    class test_LandscapeCanvas_AreaNodes_DependentComponentsAdded(EditorBatchedTest):
        from .EditorScripts import AreaNodes_DependentComponentsAdded as test_module

    class test_LandscapeCanvas_AreaNodes_EntityCreatedOnNodeAdd(EditorBatchedTest):
        from .EditorScripts import AreaNodes_EntityCreatedOnNodeAdd as test_module

    class test_LandscapeCanvas_AreaNodes_EntityRemovedOnNodeDelete(EditorBatchedTest):
        from .EditorScripts import AreaNodes_EntityRemovedOnNodeDelete as test_module

    class test_LandscapeCanvas_Component_AddedRemoved(EditorBatchedTest):
        from .EditorScripts import Component_AddedRemoved as test_module

    class test_LandscapeCanvas_ComponentUpdates_UpdateGraph(EditorBatchedTest):
        from .EditorScripts import ComponentUpdates_UpdateGraph as test_module

    class test_LandscapeCanvas_Edit_DisabledNodeDuplication(EditorSingleTest):
        from .EditorScripts import Edit_DisabledNodeDuplication as test_module

    class test_LandscapeCanvas_Edit_UndoNodeDelete_PrefabEntity(EditorBatchedTest):
        from .EditorScripts import Edit_UndoNodeDelete_PrefabEntity as test_module

    class test_LandscapeCanvas_GradientMixer_NodeConstruction(EditorBatchedTest):
        from .EditorScripts import GradientMixer_NodeConstruction as test_module

    class test_LandscapeCanvas_GradientModifierNodes_EntityCreatedOnNodeAdd(EditorBatchedTest):
        from .EditorScripts import GradientModifierNodes_EntityCreatedOnNodeAdd as test_module

    class test_LandscapeCanvas_GradientModifierNodes_EntityRemovedOnNodeDelete(EditorBatchedTest):
        from .EditorScripts import GradientModifierNodes_EntityRemovedOnNodeDelete as test_module

    class test_LandscapeCanvas_GradientNodes_DependentComponentsAdded(EditorBatchedTest):
        from .EditorScripts import GradientNodes_DependentComponentsAdded as test_module

    class test_LandscapeCanvas_GradientNodes_EntityCreatedOnNodeAdd(EditorBatchedTest):
        from .EditorScripts import GradientNodes_EntityCreatedOnNodeAdd as test_module

    class test_LandscapeCanvas_GradientNodes_EntityRemovedOnNodeDelete(EditorBatchedTest):
        from .EditorScripts import GradientNodes_EntityRemovedOnNodeDelete as test_module

    @pytest.mark.xfail(reason="https://github.com/o3de/o3de/issues/2201")
    class test_LandscapeCanvas_GraphClosed_OnEntityDelete(EditorBatchedTest):
        from .EditorScripts import GraphClosed_OnEntityDelete as test_module

    class test_LandscapeCanvas_GraphClosed_OnLevelChange(EditorBatchedTest):
        from .EditorScripts import GraphClosed_OnLevelChange as test_module

    class test_LandscapeCanvas_GraphClosed_TabbedGraphClosesIndependently(EditorBatchedTest):
        from .EditorScripts import GraphClosed_TabbedGraph as test_module

    class test_LandscapeCanvas_GraphUpdates_UpdateComponents(EditorBatchedTest):
        from .EditorScripts import GraphUpdates_UpdateComponents as test_module

    class test_LandscapeCanvas_LayerExtenderNodes_ComponentEntitySync(EditorBatchedTest):
        from .EditorScripts import LayerExtenderNodes_ComponentEntitySync as test_module

    class test_LandscapeCanvas_NewGraph_CreatedSuccessfully(EditorBatchedTest):
        from .EditorScripts import NewGraph_CreatedSuccessfully as test_module

    class test_LandscapeCanvas_Prefab_CreateInstantiate(EditorBatchedTest):
        from .EditorScripts import Prefab_CreateInstantiate as test_module

    class test_LandscapeCanvas_ShapeNodes_EntityCreatedOnNodeAdd(EditorBatchedTest):
        from .EditorScripts import ShapeNodes_EntityCreatedOnNodeAdd as test_module

    class test_LandscapeCanvas_ShapeNodes_EntityRemovedOnNodeDelete(EditorBatchedTest):
        from .EditorScripts import ShapeNodes_EntityRemovedOnNodeDelete as test_module

    class test_LandscapeCanvas_SlotConnections_UpdateComponentReferences(EditorBatchedTest):
        from .EditorScripts import SlotConnections_UpdateComponentReferences as test_module

    class test_LandscapeCanvas_ExistingTerrainSetups_GraphSuccessfully(EditorBatchedTest):
        from .EditorScripts import Terrain_ExistingSetups_GraphSuccessfully as test_module

    class test_LandscapeCanvas_Terrain_NodeConstruction(EditorBatchedTest):
        from .EditorScripts import Terrain_NodeConstruction as test_module

    class test_LandscapeCanvas_TerrainExtenderNodes_ComponentEntitySync(EditorBatchedTest):
        from .EditorScripts import TerrainExtenderNodes_ComponentEntitySync as test_module

    class test_LandscapeCanvas_TerrainNodes_DependentComponentsAdded(EditorBatchedTest):
        from .EditorScripts import TerrainNodes_DependentComponentsAdded as test_module

    class test_LandscapeCanvas_TerrainNodes_EntityCreatedOnNodeAdd(EditorBatchedTest):
        from .EditorScripts import TerrainNodes_EntityCreatedOnNodeAdd as test_module

    class test_LandscapeCanvas_TerrainNodes_EntityRemovedOnNodeDelete(EditorBatchedTest):
        from .EditorScripts import TerrainNodes_EntityRemovedOnNodeDelete as test_module
