"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
import pytest
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)) + '/../../automatedtesting_shared')
from base import TestAutomationBase


@pytest.mark.SUITE_periodic
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomation(TestAutomationBase):

    def test_GradientGenerators_Incompatibilities(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import GradientGenerators_Incompatibilities as test_module
        self._run_test(request, workspace, editor, test_module, enable_prefab_system=False)

    def test_GradientModifiers_Incompatibilities(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import GradientModifiers_Incompatibilities as test_module
        self._run_test(request, workspace, editor, test_module, enable_prefab_system=False)

    def test_GradientPreviewSettings_DefaultPinnedEntityIsSelf(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import GradientPreviewSettings_DefaultPinnedEntityIsSelf as test_module
        self._run_test(request, workspace, editor, test_module, enable_prefab_system=False)

    def test_GradientPreviewSettings_ClearingPinnedEntitySetsPreviewToOrigin(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import GradientPreviewSettings_ClearingPinnedEntitySetsPreviewToOrigin as test_module
        self._run_test(request, workspace, editor, test_module, enable_prefab_system=False)

    def test_GradientSampling_GradientReferencesAddRemoveSuccessfully(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import GradientSampling_GradientReferencesAddRemoveSuccessfully as test_module
        self._run_test(request, workspace, editor, test_module, enable_prefab_system=False)

    def test_GradientSurfaceTagEmitter_ComponentDependencies(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import GradientSurfaceTagEmitter_ComponentDependencies as test_module
        self._run_test(request, workspace, editor, test_module, enable_prefab_system=False)

    def test_GradientSurfaceTagEmitter_SurfaceTagsAddRemoveSuccessfully(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import GradientSurfaceTagEmitter_SurfaceTagsAddRemoveSuccessfully as test_module
        self._run_test(request, workspace, editor, test_module, enable_prefab_system=False)

    def test_GradientTransform_RequiresShape(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import GradientTransform_RequiresShape as test_module
        self._run_test(request, workspace, editor, test_module, enable_prefab_system=False)

    def test_GradientTransform_FrequencyZoomCanBeSetBeyondSliderRange(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import GradientTransform_FrequencyZoomCanBeSetBeyondSliderRange as test_module
        self._run_test(request, workspace, editor, test_module, enable_prefab_system=False)

    def test_GradientTransform_ComponentIncompatibleWithSpawners(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import GradientTransform_ComponentIncompatibleWithSpawners as test_module
        self._run_test(request, workspace, editor, test_module, enable_prefab_system=False)

    def test_GradientTransform_ComponentIncompatibleWithExpectedGradients(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import GradientTransform_ComponentIncompatibleWithExpectedGradients as test_module
        self._run_test(request, workspace, editor, test_module, enable_prefab_system=False)

    def test_ImageGradient_RequiresShape(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import ImageGradient_RequiresShape as test_module
        self._run_test(request, workspace, editor, test_module, enable_prefab_system=False)

    def test_ImageGradient_ProcessedImageAssignedSuccessfully(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import ImageGradient_ProcessedImageAssignedSuccessfully as test_module
        self._run_test(request, workspace, editor, test_module, enable_prefab_system=False)
