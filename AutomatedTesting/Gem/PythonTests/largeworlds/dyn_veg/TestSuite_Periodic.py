"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
import pytest
import sys

import ly_test_tools.environment.waiter as waiter
import ly_test_tools.environment.file_system as file_system
import editor_python_test_tools.hydra_test_utils as hydra
from ly_remote_console.remote_console_commands import RemoteConsole as RemoteConsole

sys.path.append(os.path.dirname(os.path.abspath(__file__)) + '/../../automatedtesting_shared')
from base import TestAutomationBase


@pytest.fixture
def remove_test_slice(request, workspace, project):
    file_system.delete([os.path.join(workspace.paths.engine_root(), project, "slices", "TestSlice_1.slice")], True,
                       True)
    file_system.delete([os.path.join(workspace.paths.engine_root(), project, "slices", "TestSlice_2.slice")], True,
                       True)

    def teardown():
        file_system.delete([os.path.join(workspace.paths.engine_root(), project, "slices", "TestSlice_1.slice")], True,
                           True)
        file_system.delete([os.path.join(workspace.paths.engine_root(), project, "slices", "TestSlice_2.slice")], True,
                           True)
    request.addfinalizer(teardown)


@pytest.fixture
def remote_console_instance(request):
    console = RemoteConsole()

    def teardown():
        if console.connected:
            console.stop()

    request.addfinalizer(teardown)
    return console


@pytest.mark.SUITE_periodic
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomation(TestAutomationBase):

    def test_AltitudeFilter_ComponentAndOverrides_InstancesPlantAtSpecifiedAltitude(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import AltitudeFilter_ComponentAndOverrides_InstancesPlantAtSpecifiedAltitude as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_AltitudeFilter_ShapeSample_InstancesPlantAtSpecifiedAltitude(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import AltitudeFilter_ShapeSample_InstancesPlantAtSpecifiedAltitude as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_AltitudeFilter_FilterStageToggle(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import AltitudeFilter_FilterStageToggle as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_SpawnerSlices_SliceCreationAndVisibilityToggleWorks(self, request, workspace, editor, remove_test_slice, launcher_platform):
        from .EditorScripts import SpawnerSlices_SliceCreationAndVisibilityToggleWorks as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_AssetListCombiner_CombinedDescriptorsExpressInConfiguredArea(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import AssetListCombiner_CombinedDescriptorsExpressInConfiguredArea as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_AssetWeightSelector_InstancesExpressBasedOnWeight(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import AssetWeightSelector_InstancesExpressBasedOnWeight as test_module
        self._run_test(request, workspace, editor, test_module)

    @pytest.mark.xfail(reason="https://github.com/o3de/o3de/issues/4155")
    def test_DistanceBetweenFilter_InstancesPlantAtSpecifiedRadius(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import DistanceBetweenFilter_InstancesPlantAtSpecifiedRadius as test_module
        self._run_test(request, workspace, editor, test_module)

    @pytest.mark.xfail(reason="https://github.com/o3de/o3de/issues/4155")
    def test_DistanceBetweenFilterOverrides_InstancesPlantAtSpecifiedRadius(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import DistanceBetweenFilterOverrides_InstancesPlantAtSpecifiedRadius as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_SurfaceDataRefreshes_RemainsStable(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import SurfaceDataRefreshes_RemainsStable as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_VegetationInstances_DespawnWhenOutOfRange(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import VegetationInstances_DespawnWhenOutOfRange as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_InstanceSpawnerPriority_LayerAndSubPriority_HigherValuesPlantOverLower(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import InstanceSpawnerPriority_LayerAndSubPriority as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_LayerBlocker_InstancesBlockedInConfiguredArea(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import LayerBlocker_InstancesBlockedInConfiguredArea as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_LayerSpawner_InheritBehaviorFlag(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import LayerSpawner_InheritBehaviorFlag as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_LayerSpawner_InstancesPlantInAllSupportedShapes(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import LayerSpawner_InstancesPlantInAllSupportedShapes as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_LayerSpawner_FilterStageToggle(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import LayerSpawner_FilterStageToggle as test_module
        self._run_test(request, workspace, editor, test_module)

    @pytest.mark.xfail(reason="https://github.com/o3de/o3de/issues/2038")
    def test_LayerSpawner_InstancesRefreshUsingCorrectViewportCamera(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import LayerSpawner_InstancesRefreshUsingCorrectViewportCamera as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_MeshBlocker_InstancesBlockedByMesh(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import MeshBlocker_InstancesBlockedByMesh as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_MeshBlocker_InstancesBlockedByMeshHeightTuning(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import MeshBlocker_InstancesBlockedByMeshHeightTuning as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_MeshSurfaceTagEmitter_DependentOnMeshComponent(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import MeshSurfaceTagEmitter_DependentOnMeshComponent as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_MeshSurfaceTagEmitter_SurfaceTagsAddRemoveSuccessfully(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import MeshSurfaceTagEmitter_SurfaceTagsAddRemoveSuccessfully as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_PhysXColliderSurfaceTagEmitter_E2E_Editor(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import PhysXColliderSurfaceTagEmitter_E2E_Editor as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_PositionModifier_ComponentAndOverrides_InstancesPlantAtSpecifiedOffsets(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import PositionModifier_ComponentAndOverrides_InstancesPlantAtSpecifiedOffsets as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_PositionModifier_AutoSnapToSurfaceWorks(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import PositionModifier_AutoSnapToSurfaceWorks as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_RotationModifier_InstancesRotateWithinRange(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import RotationModifier_InstancesRotateWithinRange as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_RotationModifierOverrides_InstancesRotateWithinRange(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import RotationModifierOverrides_InstancesRotateWithinRange as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_ScaleModifier_InstancesProperlyScale(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import ScaleModifier_InstancesProperlyScale as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_ScaleModifierOverrides_InstancesProperlyScale(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import ScaleModifierOverrides_InstancesProperlyScale as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_ShapeIntersectionFilter_InstancesPlantInAssignedShape(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import ShapeIntersectionFilter_InstancesPlantInAssignedShape as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_ShapeIntersectionFilter_FilterStageToggle(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import ShapeIntersectionFilter_FilterStageToggle as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_SlopeAlignmentModifier_InstanceSurfaceAlignment(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import SlopeAlignmentModifier_InstanceSurfaceAlignment as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_SlopeAlignmentModifierOverrides_InstanceSurfaceAlignment(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import SlopeAlignmentModifierOverrides_InstanceSurfaceAlignment as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_SurfaceMaskFilter_BasicSurfaceTagCreation(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import SurfaceMaskFilter_BasicSurfaceTagCreation as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_SurfaceMaskFilter_ExclusiveSurfaceTags_Function(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import SurfaceMaskFilter_ExclusionList as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_SurfaceMaskFilter_InclusiveSurfaceTags_Function(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import SurfaceMaskFilter_InclusionList as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_SurfaceMaskFilterOverrides_MultipleDescriptorOverridesPlantAsExpected(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import SurfaceMaskFilterOverrides_MultipleDescriptorOverridesPlantAsExpected as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_SystemSettings_SectorPointDensity(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import SystemSettings_SectorPointDensity as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_SystemSettings_SectorSize(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import SystemSettings_SectorSize as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_SlopeFilter_ComponentAndOverrides_InstancesPlantOnValidSlopes(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import SlopeFilter_ComponentAndOverrides_InstancesPlantOnValidSlope as test_module
        self._run_test(request, workspace, editor, test_module)


@pytest.mark.SUITE_periodic
@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("level", ["tmp_level"])
class TestAutomationE2E(TestAutomationBase):

    # The following tests must run in order, please do not move tests out of order

    @pytest.mark.parametrize("launcher_platform", ['windows_editor'])
    def test_DynamicSliceInstanceSpawner_Embedded_E2E_Editor(self, request, workspace, project, level, editor, launcher_platform):
        # Ensure our test level does not already exist
        file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)

        from .EditorScripts import DynamicSliceInstanceSpawner_Embedded_E2E as test_module
        self._run_test(request, workspace, editor, test_module)

    @pytest.mark.parametrize("launcher_platform", ['windows'])
    def test_DynamicSliceInstanceSpawner_Embedded_E2E_Launcher(self, workspace, launcher, level,
                                                               remote_console_instance, project, launcher_platform):

        expected_lines = [
            "Instances found in area = 400"
        ]

        hydra.launch_and_validate_results_launcher(launcher, level, remote_console_instance, expected_lines, launch_ap=False)

        # Cleanup our temp level
        file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)

    @pytest.mark.parametrize("launcher_platform", ['windows_editor'])
    def test_DynamicSliceInstanceSpawner_External_E2E_Editor(self, request, workspace, project, level, editor, launcher_platform):
        # Ensure our test level does not already exist
        file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)

        from .EditorScripts import DynamicSliceInstanceSpawner_External_E2E as test_module
        self._run_test(request, workspace, editor, test_module)

    @pytest.mark.parametrize("launcher_platform", ['windows'])
    def test_DynamicSliceInstanceSpawner_External_E2E_Launcher(self, workspace, launcher, level,
                                                               remote_console_instance, project, launcher_platform):

        expected_lines = [
            "Instances found in area = 400"
        ]

        hydra.launch_and_validate_results_launcher(launcher, level, remote_console_instance, expected_lines, launch_ap=False)

        # Cleanup our temp level
        file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)

    @pytest.mark.parametrize("launcher_platform", ['windows_editor'])
    def test_LayerBlender_E2E_Editor(self, request, workspace, project, level, editor, launcher_platform):
        # Ensure our test level does not already exist
        file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)

        from .EditorScripts import LayerBlender_E2E_Editor as test_module
        self._run_test(request, workspace, editor, test_module)

    @pytest.mark.parametrize("launcher_platform", ['windows'])
    @pytest.mark.xfail(reason="https://github.com/o3de/o3de/issues/4170")
    def test_LayerBlender_E2E_Launcher(self, workspace, launcher, level,
                                                               remote_console_instance, project, launcher_platform):

        launcher.args.extend(["-rhi=Null"])
        launcher.start(launch_ap=False)
        assert launcher.is_alive(), "Launcher failed to start"

        # Wait for test script to quit the launcher. If wait_for returns exc, test was not successful
        waiter.wait_for(lambda: not launcher.is_alive(), timeout=300)

        # Verify launcher quit successfully and did not crash
        ret_code = launcher.get_returncode()
        assert ret_code == 0, "Test failed. See Game.log for details"

        # Cleanup our temp level
        file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)
