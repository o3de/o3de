"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import pytest

from ly_test_tools.o3de.editor_test import EditorSingleTest, EditorBatchedTest, EditorParallelTest, EditorTestSuite


@pytest.mark.SUITE_periodic
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomation(EditorTestSuite):

    class test_GradientGenerators_Incompatibilities(EditorBatchedTest):
        from .EditorScripts import GradientGenerators_Incompatibilities as test_module

    class test_GradientModifiers_Incompatibilities(EditorBatchedTest):
        from .EditorScripts import GradientModifiers_Incompatibilities as test_module

    class test_GradientPreviewSettings_ClearingPinnedEntitySetsPreviewToOrigin(EditorBatchedTest):
        from .EditorScripts import GradientPreviewSettings_ClearingPinnedEntitySetsPreviewToOrigin as test_module

    class test_GradientSampling_GradientReferencesAddRemoveSuccessfully(EditorBatchedTest):
        from .EditorScripts import GradientSampling_GradientReferencesAddRemoveSuccessfully as test_module

    @pytest.mark.xfail(reason="https://github.com/o3de/o3de/issues/13134")
    class test_GradientSurfaceTagEmitter_ComponentDependencies(EditorBatchedTest):
        from .EditorScripts import GradientSurfaceTagEmitter_ComponentDependencies as test_module

    class test_GradientSurfaceTagEmitter_SurfaceTagsAddRemoveSuccessfully(EditorBatchedTest):
        from .EditorScripts import GradientSurfaceTagEmitter_SurfaceTagsAddRemoveSuccessfully as test_module

    class test_GradientTransform_ComponentIncompatibleWithExpectedGradients(EditorBatchedTest):
        from .EditorScripts import GradientTransform_ComponentIncompatibleWithExpectedGradients as test_module

    class test_GradientTransform_ComponentIncompatibleWithSpawners(EditorBatchedTest):
        from .EditorScripts import GradientTransform_ComponentIncompatibleWithSpawners as test_module

    class test_GradientTransform_FrequencyZoomCanBeSetBeyondSliderRange(EditorBatchedTest):
        from .EditorScripts import GradientTransform_FrequencyZoomCanBeSetBeyondSliderRange as test_module

    class test_GradientTransform_RequiresShape(EditorBatchedTest):
        from .EditorScripts import GradientTransform_RequiresShape as test_module

    class test_ImageGradient_BilinearFiltering(EditorBatchedTest):
        from .EditorScripts import ImageGradient_BilinearFiltering as test_module

    class test_ImageGradient_ModifiesSurfaces(EditorBatchedTest):
        from .EditorScripts import ImageGradient_ModifiesSurfaces as test_module

    class test_ImageGradient_ProcessedImageAssignedSuccessfully(EditorBatchedTest):
        from .EditorScripts import ImageGradient_ProcessedImageAssignedSuccessfully as test_module

    class test_ImageGradient_RequiresShape(EditorBatchedTest):
        from .EditorScripts import ImageGradient_RequiresShape as test_module
