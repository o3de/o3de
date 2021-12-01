"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
import pytest

import ly_test_tools.environment.file_system as file_system
from ly_test_tools.o3de.editor_test import EditorSingleTest, EditorSharedTest, EditorParallelTest, EditorTestSuite


@pytest.mark.SUITE_main
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomation(EditorTestSuite):

    enable_prefab_system = False
    
    class test_DynamicSliceInstanceSpawner_DynamicSliceSpawnerWorks(EditorParallelTest):
        from .EditorScripts import DynamicSliceInstanceSpawner_DynamicSliceSpawnerWorks as test_module

    class test_EmptyInstanceSpawner_EmptySpawnerWorks(EditorParallelTest):
        from .EditorScripts import EmptyInstanceSpawner_EmptySpawnerWorks as test_module

    class test_AltitudeFilter_ComponentAndOverrides_InstancesPlantAtSpecifiedAltitude(EditorParallelTest):
        from .EditorScripts import AltitudeFilter_ComponentAndOverrides_InstancesPlantAtSpecifiedAltitude as test_module

    class test_AltitudeFilter_ShapeSample_InstancesPlantAtSpecifiedAltitude(EditorParallelTest):
        from .EditorScripts import AltitudeFilter_ShapeSample_InstancesPlantAtSpecifiedAltitude as test_module

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

    @pytest.mark.skip(reason="https://github.com/o3de/o3de/issues/4155")
    class test_DistanceBetweenFilter_InstancesPlantAtSpecifiedRadius(EditorParallelTest):
        from .EditorScripts import DistanceBetweenFilter_InstancesPlantAtSpecifiedRadius as test_module

    @pytest.mark.skip(reason="https://github.com/o3de/o3de/issues/4155")
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

    class test_MeshBlocker_InstancesBlockedByMeshHeightTuning(EditorParallelTest):
        from .EditorScripts import MeshBlocker_InstancesBlockedByMeshHeightTuning as test_module

    class test_MeshSurfaceTagEmitter_DependentOnMeshComponent(EditorParallelTest):
        from .EditorScripts import MeshSurfaceTagEmitter_DependentOnMeshComponent as test_module

    class test_MeshSurfaceTagEmitter_SurfaceTagsAddRemoveSuccessfully(EditorParallelTest):
        from .EditorScripts import MeshSurfaceTagEmitter_SurfaceTagsAddRemoveSuccessfully as test_module

    class test_PhysXColliderSurfaceTagEmitter_E2E_Editor(EditorParallelTest):
        from .EditorScripts import PhysXColliderSurfaceTagEmitter_E2E_Editor as test_module

    class test_PositionModifier_ComponentAndOverrides_InstancesPlantAtSpecifiedOffsets(EditorParallelTest):
        from .EditorScripts import PositionModifier_ComponentAndOverrides_InstancesPlantAtSpecifiedOffsets as test_module

    class test_PositionModifier_AutoSnapToSurfaceWorks(EditorParallelTest):
        from .EditorScripts import PositionModifier_AutoSnapToSurfaceWorks as test_module

    class test_RotationModifier_InstancesRotateWithinRange(EditorParallelTest):
        from .EditorScripts import RotationModifier_InstancesRotateWithinRange as test_module

    class test_RotationModifierOverrides_InstancesRotateWithinRange(EditorParallelTest):
        from .EditorScripts import RotationModifierOverrides_InstancesRotateWithinRange as test_module

    class test_ScaleModifier_InstancesProperlyScale(EditorParallelTest):
        from .EditorScripts import ScaleModifier_InstancesProperlyScale as test_module

    class test_ScaleModifierOverrides_InstancesProperlyScale(EditorParallelTest):
        from .EditorScripts import ScaleModifierOverrides_InstancesProperlyScale as test_module

    class test_ShapeIntersectionFilter_InstancesPlantInAssignedShape(EditorParallelTest):
        from .EditorScripts import ShapeIntersectionFilter_InstancesPlantInAssignedShape as test_module

    class test_ShapeIntersectionFilter_FilterStageToggle(EditorParallelTest):
        from .EditorScripts import ShapeIntersectionFilter_FilterStageToggle as test_module

    class test_SlopeAlignmentModifier_InstanceSurfaceAlignment(EditorParallelTest):
        from .EditorScripts import SlopeAlignmentModifier_InstanceSurfaceAlignment as test_module

    class test_SlopeAlignmentModifierOverrides_InstanceSurfaceAlignment(EditorParallelTest):
        from .EditorScripts import SlopeAlignmentModifierOverrides_InstanceSurfaceAlignment as test_module

    class test_SurfaceMaskFilter_BasicSurfaceTagCreation(EditorParallelTest):
        from .EditorScripts import SurfaceMaskFilter_BasicSurfaceTagCreation as test_module

    class test_SurfaceMaskFilter_ExclusiveSurfaceTags_Function(EditorParallelTest):
        from .EditorScripts import SurfaceMaskFilter_ExclusionList as test_module

    class test_SurfaceMaskFilter_InclusiveSurfaceTags_Function(EditorParallelTest):
        from .EditorScripts import SurfaceMaskFilter_InclusionList as test_module

    class test_SurfaceMaskFilterOverrides_MultipleDescriptorOverridesPlantAsExpected(EditorParallelTest):
        from .EditorScripts import SurfaceMaskFilterOverrides_MultipleDescriptorOverridesPlantAsExpected as test_module

    class test_SystemSettings_SectorPointDensity(EditorParallelTest):
        from .EditorScripts import SystemSettings_SectorPointDensity as test_module

    class test_SystemSettings_SectorSize(EditorParallelTest):
        from .EditorScripts import SystemSettings_SectorSize as test_module

    class test_SlopeFilter_ComponentAndOverrides_InstancesPlantOnValidSlopes(EditorParallelTest):
        from .EditorScripts import SlopeFilter_ComponentAndOverrides_InstancesPlantOnValidSlope as test_module

    class test_DynamicSliceInstanceSpawner_Embedded_E2E_Editor(EditorSingleTest):
        from .EditorScripts import DynamicSliceInstanceSpawner_Embedded_E2E as test_module

        # Custom teardown to remove test level created during test
        def teardown(self, request, workspace, editor, editor_test_results, launcher_platform):
            file_system.delete([os.path.join(workspace.paths.engine_root(), "AutomatedTesting", "Levels", "tmp_level")],
                               True, True)

    class test_DynamicSliceInstanceSpawner_External_E2E_Editor(EditorSingleTest):
        from .EditorScripts import DynamicSliceInstanceSpawner_External_E2E as test_module

        # Custom teardown to remove test level created during test
        def teardown(self, request, workspace, editor, editor_test_results, launcher_platform):
            file_system.delete([os.path.join(workspace.paths.engine_root(), "AutomatedTesting", "Levels", "tmp_level")],
                               True, True)
        
    class test_LayerBlender_E2E_Editor(EditorSingleTest):
        from .EditorScripts import LayerBlender_E2E_Editor as test_module

        # Custom teardown to remove test level created during test
        def teardown(self, request, workspace, editor, editor_test_results, launcher_platform):
            file_system.delete([os.path.join(workspace.paths.engine_root(), "AutomatedTesting", "Levels", "tmp_level")],
                               True, True)
