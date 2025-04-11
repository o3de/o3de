"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
import pytest

import ly_test_tools.environment.file_system as file_system
from ly_test_tools.o3de.editor_test import EditorSingleTest, EditorBatchedTest, EditorParallelTest, EditorTestSuite


@pytest.mark.SUITE_main
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomation(EditorTestSuite):

    # Helpers for test asset cleanup
    def cleanup_test_level(self, workspace):
        file_system.delete([os.path.join(workspace.paths.engine_root(), "AutomatedTesting", "Levels", "tmp_level")],
                           True, True)

    class test_AltitudeFilter_Component(EditorBatchedTest):
        from .EditorScripts import AltitudeFilter_ComponentAndOverrides_InstancesPlantAtSpecifiedAltitude as test_module
    
    @pytest.mark.skip(reason="https://github.com/o3de/o3de/issues/17337")
    class test_AltitudeFilter_Toggle(EditorBatchedTest):
        from .EditorScripts import AltitudeFilter_FilterStageToggle as test_module

    class test_AltitudeFilter_Shape(EditorBatchedTest):
        from .EditorScripts import AltitudeFilter_ShapeSample_InstancesPlantAtSpecifiedAltitude as test_module

    class test_AssetListCombiner(EditorBatchedTest):
        from .EditorScripts import AssetListCombiner_CombinedDescriptorsExpressInConfiguredArea as test_module

    class test_AssetWeightSelector(EditorBatchedTest):
        from .EditorScripts import AssetWeightSelector_InstancesExpressBasedOnWeight as test_module

    @pytest.mark.skip(reason="https://github.com/o3de/o3de/issues/4155")
    class test_DistanceBetweenFilter_InstancesPlantAtSpecifiedRadius(EditorBatchedTest):
        from .EditorScripts import DistanceBetweenFilter_InstancesPlantAtSpecifiedRadius as test_module

    @pytest.mark.skip(reason="https://github.com/o3de/o3de/issues/4155")
    class test_DistanceBetweenFilterOverrides_InstancesPlantAtSpecifiedRadius(EditorBatchedTest):
        from .EditorScripts import DistanceBetweenFilterOverrides_InstancesPlantAtSpecifiedRadius as test_module

    class test_EmptyInstanceSpawner_EmptySpawnerWorks(EditorBatchedTest):
        from .EditorScripts import EmptyInstanceSpawner_EmptySpawnerWorks as test_module

    class test_InstanceSpawnerPriority(EditorBatchedTest):
        from .EditorScripts import InstanceSpawnerPriority_LayerAndSubPriority as test_module

    class test_LayerBlender_E2E_Editor(EditorSingleTest):
        from .EditorScripts import LayerBlender_E2E_Editor as test_module

        # Custom setup/teardown to remove test level created during test
        def setup(self, request, workspace):
            TestAutomation.cleanup_test_level(self, workspace)

        def teardown(self, request, workspace, editor_test_results):
            TestAutomation.cleanup_test_level(self, workspace)

    class test_LayerBlocker(EditorBatchedTest):
        from .EditorScripts import LayerBlocker_InstancesBlockedInConfiguredArea as test_module

    @pytest.mark.skip(reason="https://github.com/o3de/o3de/issues/17337")
    class test_LayerSpawner_Toggle(EditorBatchedTest):
        from .EditorScripts import LayerSpawner_FilterStageToggle as test_module

    class test_LayerSpawner_InheritBehavior(EditorBatchedTest):
        from .EditorScripts import LayerSpawner_InheritBehaviorFlag as test_module

    class test_LayerSpawner_Plant(EditorBatchedTest):
        from .EditorScripts import LayerSpawner_InstancesPlantInAllSupportedShapes as test_module

    @pytest.mark.xfail(reason="https://github.com/o3de/o3de/issues/6549")
    class test_LayerSpawner_InstancesRefreshUsingCorrectViewportCamera(EditorBatchedTest):
        from .EditorScripts import LayerSpawner_InstancesRefreshUsingCorrectViewportCamera as test_module

    class test_MeshBlocker_Mesh(EditorBatchedTest):
        from .EditorScripts import MeshBlocker_InstancesBlockedByMesh as test_module

    class test_MeshBlocker_Height(EditorBatchedTest):
        from .EditorScripts import MeshBlocker_InstancesBlockedByMeshHeightTuning as test_module

    class test_MeshSurfaceTagEmitter_Mesh(EditorBatchedTest):
        from .EditorScripts import MeshSurfaceTagEmitter_DependentOnMeshComponent as test_module

    class test_MeshSurfaceTagEmitter_Tags(EditorBatchedTest):
        from .EditorScripts import MeshSurfaceTagEmitter_SurfaceTagsAddRemoveSuccessfully as test_module

    class test_SurfaceTagEmitter(EditorBatchedTest):
        from .EditorScripts import PhysXColliderSurfaceTagEmitter_E2E_Editor as test_module

    @pytest.mark.skip(reason="https://github.com/o3de/o3de/issues/17337")
    class test_PositionModifier_Snap(EditorBatchedTest):
        from .EditorScripts import PositionModifier_AutoSnapToSurfaceWorks as test_module

    class test_PositionModifier_Plant(EditorBatchedTest):
        from .EditorScripts import \
            PositionModifier_ComponentAndOverrides_InstancesPlantAtSpecifiedOffsets as test_module

    class test_PrefabSpawner_Embedded(EditorSingleTest):
        from .EditorScripts import PrefabInstanceSpawner_Embedded_E2E as test_module

        # Custom setup/teardown to remove test level created during test
        def setup(self, request, workspace):
            TestAutomation.cleanup_test_level(self, workspace)

        def teardown(self, request, workspace, editor_test_results):
            TestAutomation.cleanup_test_level(self, workspace)

    class test_PrefabSpawner_External(EditorSingleTest):
        from .EditorScripts import PrefabInstanceSpawner_External_E2E as test_module

        # Custom setup/teardown to remove test level created during test
        def setup(self, request, workspace):
            TestAutomation.cleanup_test_level(self, workspace)

        def teardown(self, request, workspace, editor_test_results):
            TestAutomation.cleanup_test_level(self, workspace)

    class test_RotationModifier(EditorBatchedTest):
        from .EditorScripts import RotationModifier_InstancesRotateWithinRange as test_module

    class test_RotationModifierOverrides(EditorBatchedTest):
        from .EditorScripts import RotationModifierOverrides_InstancesRotateWithinRange as test_module

    class test_ScaleModifier(EditorBatchedTest):
        from .EditorScripts import ScaleModifier_InstancesProperlyScale as test_module

    class test_ScaleModifierOverrides(EditorBatchedTest):
        from .EditorScripts import ScaleModifierOverrides_InstancesProperlyScale as test_module

    class test_ShapeFilter_Toggle(EditorBatchedTest):
        from .EditorScripts import ShapeIntersectionFilter_FilterStageToggle as test_module

    class test_ShapeFilter_Plant(EditorBatchedTest):
        from .EditorScripts import ShapeIntersectionFilter_InstancesPlantInAssignedShape as test_module

    class test_SlopeAlignmentModifier(EditorBatchedTest):
        from .EditorScripts import SlopeAlignmentModifier_InstanceSurfaceAlignment as test_module

    class test_SlopeAlignmentModifierOverrides(EditorBatchedTest):
        from .EditorScripts import SlopeAlignmentModifierOverrides_InstanceSurfaceAlignment as test_module

    @pytest.mark.skip(reason="https://github.com/o3de/o3de/issues/17337")
    class test_SlopeFilter_Plant(EditorBatchedTest):
        from .EditorScripts import SlopeFilter_ComponentAndOverrides_InstancesPlantOnValidSlope as test_module

    class test_SpawnerPrefabs_Toggle(EditorBatchedTest):
        from .EditorScripts import SpawnerPrefabs_PrefabCreationAndVisibilityToggleWorks as test_module

    class test_SurfaceDataRefreshes(EditorBatchedTest):
        from .EditorScripts import SurfaceDataRefreshes_RemainsStable as test_module

    class test_SurfaceMaskFilter_BasicTag(EditorBatchedTest):
        from .EditorScripts import SurfaceMaskFilter_BasicSurfaceTagCreation as test_module

    class test_SurfaceMaskFilter_ExclusiveTag(EditorBatchedTest):
        from .EditorScripts import SurfaceMaskFilter_ExclusionList as test_module

    class test_SurfaceMaskFilter_InclusiveTag(EditorBatchedTest):
        from .EditorScripts import SurfaceMaskFilter_InclusionList as test_module

    class test_SurfaceMaskFilterOverrides(EditorBatchedTest):
        from .EditorScripts import SurfaceMaskFilterOverrides_MultipleDescriptorOverridesPlantAsExpected as test_module

    class test_SystemSettings_SectorPointDensity(EditorBatchedTest):
        from .EditorScripts import SystemSettings_SectorPointDensity as test_module

    class test_SystemSettings_SectorSize(EditorBatchedTest):
        from .EditorScripts import SystemSettings_SectorSize as test_module

    class test_VegetationInstances_DespawnWhenOutOfRange(EditorBatchedTest):
        from .EditorScripts import VegetationInstances_DespawnWhenOutOfRange as test_module
