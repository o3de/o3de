"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import pytest

from ly_test_tools.o3de.editor_test import EditorSingleTest, EditorSharedTest, EditorParallelTest, EditorTestSuite


@pytest.mark.SUITE_periodic
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomation(EditorTestSuite):

    class test_LandscapeCanvas_AreaNodes_DependentComponentsAdded(EditorSharedTest):
        from .EditorScripts import AreaNodes_DependentComponentsAdded as test_module

    class test_LandscapeCanvas_AreaNodes_EntityCreatedOnNodeAdd(EditorSharedTest):
        from .EditorScripts import AreaNodes_EntityCreatedOnNodeAdd as test_module

    class test_LandscapeCanvas_AreaNodes_EntityRemovedOnNodeDelete(EditorSharedTest):
        from .EditorScripts import AreaNodes_EntityRemovedOnNodeDelete as test_module

    class test_LandscapeCanvas_LayerExtenderNodes_ComponentEntitySync(EditorSharedTest):
        from .EditorScripts import LayerExtenderNodes_ComponentEntitySync as test_module

    class test_LandscapeCanvas_Edit_DisabledNodeDuplication(EditorSharedTest):
        from .EditorScripts import Edit_DisabledNodeDuplication as test_module

    class test_LandscapeCanvas_Edit_UndoNodeDelete_SliceEntity(EditorSharedTest):
        from .EditorScripts import Edit_UndoNodeDelete_SliceEntity as test_module

    class test_LandscapeCanvas_NewGraph_CreatedSuccessfully(EditorSharedTest):
        from .EditorScripts import NewGraph_CreatedSuccessfully as test_module

    class test_LandscapeCanvas_Component_AddedRemoved(EditorSharedTest):
        from .EditorScripts import Component_AddedRemoved as test_module

    class test_LandscapeCanvas_GraphClosed_OnLevelChange(EditorSharedTest):
        from .EditorScripts import GraphClosed_OnLevelChange as test_module
