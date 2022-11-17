"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import pytest

from ly_test_tools.o3de.editor_test import EditorTestSuite, EditorBatchedTest


@pytest.mark.SUITE_main
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomation(EditorTestSuite):

    class test_AxisAlignedBoxShape_ConfigurationWorks(EditorBatchedTest):
        from .EditorScripts import TerrainPhysicsCollider_ChangesSizeWithAxisAlignedBoxShapeChanges as test_module

    @pytest.mark.skip(reason="GHI #9850: Test Periodically Fails")
    class test_Terrain_SupportsPhysics(EditorBatchedTest):
        from .EditorScripts import Terrain_SupportsPhysics as test_module

    class test_TerrainHeightGradientList_AddRemoveGradientWorks(EditorBatchedTest):
        from .EditorScripts import TerrainHeightGradientList_AddRemoveGradientWorks as test_module
        
    class test_TerrainSystem_VegetationSpawnsOnTerrainSurfaces(EditorBatchedTest):
        from .EditorScripts import TerrainSystem_VegetationSpawnsOnTerrainSurfaces as test_module

    class test_TerrainMacroMaterialComponent_MacroMaterialActivates(EditorBatchedTest):
         from .EditorScripts import TerrainMacroMaterialComponent_MacroMaterialActivates as test_module

    class test_TerrainWorld_ConfigurationWorks(EditorBatchedTest):
        from .EditorScripts import Terrain_World_ConfigurationWorks as test_module
