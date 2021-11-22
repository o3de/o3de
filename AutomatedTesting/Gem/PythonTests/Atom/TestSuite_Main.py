"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import pytest

from ly_test_tools.o3de.editor_test import EditorSharedTest, EditorTestSuite


@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
class TestAutomation(EditorTestSuite):

    enable_prefab_system = False

    @pytest.mark.test_case_id("C36525657")
    class AtomEditorComponents_BloomAdded(EditorSharedTest):
        from Atom.tests import hydra_AtomEditorComponents_BloomAdded as test_module

    @pytest.mark.test_case_id("C32078118")
    class AtomEditorComponents_DecalAdded(EditorSharedTest):
        from Atom.tests import hydra_AtomEditorComponents_DecalAdded as test_module

    @pytest.mark.test_case_id("C36525658")
    class AtomEditorComponents_DeferredFogAdded(EditorSharedTest):
        from Atom.tests import hydra_AtomEditorComponents_DeferredFogAdded as test_module

    @pytest.mark.test_case_id("C32078119")
    class AtomEditorComponents_DepthOfFieldAdded(EditorSharedTest):
        from Atom.tests import hydra_AtomEditorComponents_DepthOfFieldAdded as test_module

    @pytest.mark.test_case_id("C36525659")
    class AtomEditorComponents_DiffuseProbeGridAdded(EditorSharedTest):
        from Atom.tests import hydra_AtomEditorComponents_DiffuseProbeGridAdded as test_module

    @pytest.mark.test_case_id("C32078120")
    class AtomEditorComponents_DirectionalLightAdded(EditorSharedTest):
        from Atom.tests import hydra_AtomEditorComponents_DirectionalLightAdded as test_module

    @pytest.mark.test_case_id("C36525660")
    class AtomEditorComponents_DisplayMapperAdded(EditorSharedTest):
        from Atom.tests import hydra_AtomEditorComponents_DisplayMapperAdded as test_module

    @pytest.mark.test_case_id("C36525661")
    class AtomEditorComponents_EntityReferenceAdded(EditorSharedTest):
        from Atom.tests import hydra_AtomEditorComponents_EntityReferenceAdded as test_module

    @pytest.mark.test_case_id("C32078121")
    class AtomEditorComponents_ExposureControlAdded(EditorSharedTest):
        from Atom.tests import hydra_AtomEditorComponents_ExposureControlAdded as test_module

    @pytest.mark.test_case_id("C32078115")
    class AtomEditorComponents_GlobalSkylightIBLAdded(EditorSharedTest):
        from Atom.tests import hydra_AtomEditorComponents_GlobalSkylightIBLAdded as test_module
        
    @pytest.mark.test_case_id("C32078122")
    class AtomEditorComponents_GridAdded(EditorSharedTest):
        from Atom.tests import hydra_AtomEditorComponents_GridAdded as test_module

    @pytest.mark.test_case_id("C36525671")
    class AtomEditorComponents_HDRColorGradingAdded(EditorSharedTest):
        from Atom.tests import hydra_AtomEditorComponents_HDRColorGradingAdded as test_module

    @pytest.mark.test_case_id("C32078116")
    class AtomEditorComponents_HDRiSkyboxAdded(EditorSharedTest):
        from Atom.tests import hydra_AtomEditorComponents_HDRiSkyboxAdded as test_module

    @pytest.mark.test_case_id("C32078117")
    class AtomEditorComponents_LightAdded(EditorSharedTest):
        from Atom.tests import hydra_AtomEditorComponents_LightAdded as test_module

    @pytest.mark.test_case_id("C36525662")
    class AtomEditorComponents_LookModificationAdded(EditorSharedTest):
        from Atom.tests import hydra_AtomEditorComponents_LookModificationAdded as test_module

    @pytest.mark.test_case_id("C32078123")
    class AtomEditorComponents_MaterialAdded(EditorSharedTest):
        from Atom.tests import hydra_AtomEditorComponents_MaterialAdded as test_module

    @pytest.mark.test_case_id("C32078124")
    class AtomEditorComponents_MeshAdded(EditorSharedTest):
        from Atom.tests import hydra_AtomEditorComponents_MeshAdded as test_module

    @pytest.mark.test_case_id("C36525663")
    class AtomEditorComponents_OcclusionCullingPlaneAdded(EditorSharedTest):
        from Atom.tests import hydra_AtomEditorComponents_OcclusionCullingPlaneAdded as test_module

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

    @pytest.mark.test_case_id("C36525666")
    class AtomEditorComponents_SSAOAdded(EditorSharedTest):
        from Atom.tests import hydra_AtomEditorComponents_SSAOAdded as test_module

    class ShaderAssetBuilder_RecompilesShaderAsChainOfDependenciesChanges(EditorSharedTest):
        from Atom.tests import hydra_ShaderAssetBuilder_RecompilesShaderAsChainOfDependenciesChanges as test_module
