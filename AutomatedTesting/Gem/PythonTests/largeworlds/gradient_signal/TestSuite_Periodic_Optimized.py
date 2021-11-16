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

    enable_prefab_system = False

    class test_GradientGenerators_Incompatibilities(EditorSharedTest):
        from .EditorScripts import GradientGenerators_Incompatibilities as test_module

    class test_GradientModifiers_Incompatibilities(EditorSharedTest):
        from .EditorScripts import GradientModifiers_Incompatibilities as test_module

    class test_GradientPreviewSettings_DefaultPinnedEntityIsSelf(EditorSharedTest):
        from .EditorScripts import GradientPreviewSettings_DefaultPinnedEntityIsSelf as test_module

    class test_GradientPreviewSettings_ClearingPinnedEntitySetsPreviewToOrigin(EditorSharedTest):
        from .EditorScripts import GradientPreviewSettings_ClearingPinnedEntitySetsPreviewToOrigin as test_module

    class test_GradientSampling_GradientReferencesAddRemoveSuccessfully(EditorSharedTest):
        from .EditorScripts import GradientSampling_GradientReferencesAddRemoveSuccessfully as test_module

    class test_GradientSurfaceTagEmitter_ComponentDependencies(EditorSharedTest):
        from .EditorScripts import GradientSurfaceTagEmitter_ComponentDependencies as test_module

    class test_GradientSurfaceTagEmitter_SurfaceTagsAddRemoveSuccessfully(EditorSharedTest):
        from .EditorScripts import GradientSurfaceTagEmitter_SurfaceTagsAddRemoveSuccessfully as test_module

    class test_GradientTransform_RequiresShape(EditorSharedTest):
        from .EditorScripts import GradientTransform_RequiresShape as test_module

    class test_GradientTransform_FrequencyZoomCanBeSetBeyondSliderRange(EditorSharedTest):
        from .EditorScripts import GradientTransform_FrequencyZoomCanBeSetBeyondSliderRange as test_module

    class test_GradientTransform_ComponentIncompatibleWithSpawners(EditorSharedTest):
        from .EditorScripts import GradientTransform_ComponentIncompatibleWithSpawners as test_module

    class test_GradientTransform_ComponentIncompatibleWithExpectedGradients(EditorSharedTest):
        from .EditorScripts import GradientTransform_ComponentIncompatibleWithExpectedGradients as test_module

    class test_ImageGradient_RequiresShape(EditorSharedTest):
        from .EditorScripts import ImageGradient_RequiresShape as test_module

    class test_ImageGradient_ProcessedImageAssignedSuccessfully(EditorSharedTest):
        from .EditorScripts import ImageGradient_ProcessedImageAssignedSuccessfully as test_module
