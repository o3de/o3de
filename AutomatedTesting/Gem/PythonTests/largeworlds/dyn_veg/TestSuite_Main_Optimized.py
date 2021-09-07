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
class TestAutomation(EditorTestSuite):

    class test_DynamicSliceInstanceSpawner_DynamicSliceSpawnerWorks(EditorParallelTest):
        from .EditorScripts import DynamicSliceInstanceSpawner_DynamicSliceSpawnerWorks as test_module

    class test_EmptyInstanceSpawner_EmptySpawnerWorks(EditorParallelTest):
        from .EditorScripts import EmptyInstanceSpawner_EmptySpawnerWorks as test_module

    class test_AltitudeFilter_ComponentAndOverrides_InstancesPlantAtSpecifiedAltitude(EditorParallelTest):
        from .EditorScripts import AltitudeFilter_ComponentAndOverrides_InstancesPlantAtSpecifiedAltitude as test_module

    class test_AltitudeFilter_ShapeSample_InstancesPlantAtSpecifiedAltitude(EditorParallelTest):
        from .EditorScripts import AltitudeFilter_ShapeSample_InstancesPlantAtSpecifiedAltitude as test_module

    @pytest.mark.xfail(reason="https://github.com/o3de/o3de/issues/2303")
    class test_AltitudeFilter_FilterStageToggle(EditorParallelTest):
        from .EditorScripts import AltitudeFilter_FilterStageToggle as test_module

    class test_SpawnerSlices_SliceCreationAndVisibilityToggleWorks(EditorSingleTest):
        # Custom teardown to remove slice asset created during test
        def teardown(self, request, workspace, editor, editor_test_results, launcher_platform):
            file_system.delete([os.path.join(workspace.paths.engine_root(), "AutomatedTesting", "slices",
                                             "TestSlice_1.slice")], True, True)
            file_system.delete([os.path.join(workspace.paths.engine_root(), "AutomatedTesting", "slices",
                                             "TestSlice_2.slice")], True, True)
        from .EditorScripts import SpawnerSlices_SliceCreationAndVisibilityToggleWorks as test_module

    class test_AssetListCombiner_CombinedDescriptorsExpressInConfiguredArea(EditorParallelTest):
        from .EditorScripts import AssetListCombiner_CombinedDescriptorsExpressInConfiguredArea as test_module

    class test_AssetWeightSelector_InstancesExpressBasedOnWeight(EditorParallelTest):
        from .EditorScripts import AssetWeightSelector_InstancesExpressBasedOnWeight as test_module

    class test_DistanceBetweenFilter_InstancesPlantAtSpecifiedRadius(EditorParallelTest):
        from .EditorScripts import DistanceBetweenFilter_InstancesPlantAtSpecifiedRadius as test_module

    class test_DistanceBetweenFilterOverrides_InstancesPlantAtSpecifiedRadius(EditorParallelTest):
        from .EditorScripts import DistanceBetweenFilterOverrides_InstancesPlantAtSpecifiedRadius as test_module

    class test_SurfaceDataRefreshes_RemainsStable(EditorParallelTest):
        from .EditorScripts import SurfaceDataRefreshes_RemainsStable as test_module

    class test_VegetationInstances_DespawnWhenOutOfRange(EditorParallelTest):
        from .EditorScripts import VegetationInstances_DespawnWhenOutOfRange as test_module

    class test_InstanceSpawnerPriority_LayerAndSubPriority_HigherValuesPlantOverLower(EditorParallelTest):
        from .EditorScripts import InstanceSpawnerPriority_LayerAndSubPriority as test_module

    class test_LayerBlocker_InstancesBlockedInConfiguredArea(EditorParallelTest):
        from .EditorScripts import LayerBlocker_InstancesBlockedInConfiguredArea as test_module

    class test_LayerSpawner_InheritBehaviorFlag(EditorParallelTest):
        from .EditorScripts import LayerSpawner_InheritBehaviorFlag as test_module

    class test_LayerSpawner_InstancesPlantInAllSupportedShapes(EditorParallelTest):
        from .EditorScripts import LayerSpawner_InstancesPlantInAllSupportedShapes as test_module

    class test_LayerSpawner_FilterStageToggle(EditorParallelTest):
        from .EditorScripts import LayerSpawner_FilterStageToggle as test_module

    @pytest.mark.xfail(reason="https://github.com/o3de/o3de/issues/2038")
    class test_LayerSpawner_InstancesRefreshUsingCorrectViewportCamera(EditorParallelTest):
        from .EditorScripts import LayerSpawner_InstancesRefreshUsingCorrectViewportCamera as test_module

    class test_MeshBlocker_InstancesBlockedByMesh(EditorParallelTest):
        from .EditorScripts import MeshBlocker_InstancesBlockedByMesh as test_module
