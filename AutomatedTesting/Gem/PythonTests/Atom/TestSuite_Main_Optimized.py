"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import pytest

from ly_test_tools.o3de.editor_test import EditorSharedTest, EditorTestSuite


@pytest.mark.xfail(reason="Optimized tests are experimental, we will enable xfail and monitor them temporarily.")
@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
class TestAutomation(EditorTestSuite):

    @pytest.mark.test_case_id("C32078118")
    class AtomEditorComponents_DecalAdded(EditorSharedTest):
        from Atom.tests import hydra_AtomEditorComponents_DecalAdded as test_module

    @pytest.mark.test_case_id("C32078119")
    class AtomEditorComponents_DepthOfFieldAdded(EditorSharedTest):
        from Atom.tests import hydra_AtomEditorComponents_DepthOfFieldAdded as test_module

    @pytest.mark.test_case_id("C32078120")
    class AtomEditorComponents_DirectionalLightAdded(EditorSharedTest):
        from Atom.tests import hydra_AtomEditorComponents_DirectionalLightAdded as test_module

    @pytest.mark.test_case_id("C36525660")
    class AtomEditorComponents_DisplayMapperAdded(EditorSharedTest):
        from Atom.tests import hydra_AtomEditorComponents_DisplayMapperAdded as test_module

    @pytest.mark.test_case_id("C32078121")
    class AtomEditorComponents_ExposureControlAdded(EditorSharedTest):
        from Atom.tests import hydra_AtomEditorComponents_ExposureControlAdded as test_module

    @pytest.mark.test_case_id("C32078115")
    class AtomEditorComponents_GlobalSkylightIBLAdded(EditorSharedTest):
        from Atom.tests import hydra_AtomEditorComponents_GlobalSkylightIBLAdded as test_module

    @pytest.mark.test_case_id("C32078117")
    class AtomEditorComponents_LightAdded(EditorSharedTest):
        from Atom.tests import hydra_AtomEditorComponents_LightAdded as test_module

    @pytest.mark.test_case_id("C32078123")
    class AtomEditorComponents_MaterialAdded(EditorSharedTest):
        from Atom.tests import hydra_AtomEditorComponents_MaterialAdded as test_module

    @pytest.mark.test_case_id("C32078124")
    class AtomEditorComponents_MeshAdded(EditorSharedTest):
        from Atom.tests import hydra_AtomEditorComponents_MeshAdded as test_module

    @pytest.mark.test_case_id("C32078125")
    class AtomEditorComponents_PhysicalSkyAdded(EditorSharedTest):
        from Atom.tests import hydra_AtomEditorComponents_PhysicalSkyAdded as test_module

    @pytest.mark.test_case_id("C36525664")
    class AtomEditorComponents_PostFXGradientWeightModifierAdded(EditorSharedTest):
        from Atom.tests import hydra_AtomEditorComponents_PostFXGradientWeightModifierAdded as test_module

    @pytest.mark.test_case_id("C32078127")
    class AtomEditorComponents_PostFXLayerAdded(EditorSharedTest):
        from Atom.tests import hydra_AtomEditorComponents_PostFXLayerAdded as test_module

    @pytest.mark.test_case_id("C32078131")
    class AtomEditorComponents_PostFXRadiusWeightModifierAdded(EditorSharedTest):
        from Atom.tests import (
            hydra_AtomEditorComponents_PostFXRadiusWeightModifierAdded as test_module)

    @pytest.mark.test_case_id("C36525665")
    class AtomEditorComponents_PostFXShapeWeightModifierAdded(EditorSharedTest):
        from Atom.tests import hydra_AtomEditorComponents_PostFxShapeWeightModifierAdded as test_module

    @pytest.mark.test_case_id("C32078128")
    class AtomEditorComponents_ReflectionProbeAdded(EditorSharedTest):
        from Atom.tests import hydra_AtomEditorComponents_ReflectionProbeAdded as test_module

    class ShaderAssetBuilder_RecompilesShaderAsChainOfDependenciesChanges(EditorSharedTest):
        from Atom.tests import hydra_ShaderAssetBuilder_RecompilesShaderAsChainOfDependenciesChanges as test_module
