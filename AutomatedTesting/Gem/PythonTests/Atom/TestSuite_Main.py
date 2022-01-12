"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import logging
import os
import pytest

import ly_test_tools.environment.file_system as file_system
import editor_python_test_tools.hydra_test_utils as hydra

from ly_test_tools.o3de.editor_test import EditorSharedTest, EditorTestSuite

logger = logging.getLogger(__name__)
TEST_DIRECTORY = os.path.join(os.path.dirname(__file__), "tests")


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


@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("launcher_platform", ['windows_generic'])
class TestMaterialEditorBasicTests(object):
    @pytest.fixture(autouse=True)
    def setup_teardown(self, request, workspace, project):
        def delete_files():
            file_system.delete(
                    [
                        os.path.join(workspace.paths.project(), "Materials", "test_material.material"),
                        os.path.join(workspace.paths.project(), "Materials", "test_material_1.material"),
                        os.path.join(workspace.paths.project(), "Materials", "test_material_2.material"),
                    ],
                    True,
                    True,
                )
        # Cleanup our newly created materials
        delete_files()

        def teardown():
            # Cleanup our newly created materials
            delete_files()

        request.addfinalizer(teardown)

    @pytest.mark.parametrize("exe_file_name", ["MaterialEditor"])
    @pytest.mark.test_case_id("C34448113")  # Creating a New Asset.
    @pytest.mark.test_case_id("C34448114")  # Opening an Existing Asset.
    @pytest.mark.test_case_id("C34448115")  # Closing Selected Material.
    @pytest.mark.test_case_id("C34448116")  # Closing All Materials.
    @pytest.mark.test_case_id("C34448117")  # Closing all but Selected Material.
    @pytest.mark.test_case_id("C34448118")  # Saving Material.
    @pytest.mark.test_case_id("C34448119")  # Saving as a New Material.
    @pytest.mark.test_case_id("C34448120")  # Saving as a Child Material.
    @pytest.mark.test_case_id("C34448121")  # Saving all Open Materials.
    def test_MaterialEditorBasicTests(
            self, request, workspace, project, launcher_platform, generic_launcher, exe_file_name):

        expected_lines = [
            "Material opened: True",
            "Test asset doesn't exist initially: True",
            "New asset created: True",
            "New Material opened: True",
            "Material closed: True",
            "All documents closed: True",
            "Close All Except Selected worked as expected: True",
            "Actual Document saved with changes: True",
            "Document saved as copy is saved with changes: True",
            "Document saved as child is saved with changes: True",
            "Save All worked as expected: True",
        ]
        unexpected_lines = [
            "Traceback (most recent call last):"
        ]

        hydra.launch_and_validate_results(
            request,
            TEST_DIRECTORY,
            generic_launcher,
            "hydra_AtomMaterialEditor_BasicTests.py",
            run_python="--runpython",
            timeout=43,
            expected_lines=expected_lines,
            unexpected_lines=unexpected_lines,
            halt_on_unexpected=True,
            null_renderer=True,
            log_file_name="MaterialEditor.log",
            enable_prefab_system=False,
        )
