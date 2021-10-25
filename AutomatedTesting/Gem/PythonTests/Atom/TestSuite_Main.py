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
from Atom.atom_utils.atom_constants import LIGHT_TYPES

logger = logging.getLogger(__name__)
TEST_DIRECTORY = os.path.join(os.path.dirname(__file__), "tests")


@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("level", ["auto_test"])
class TestAtomEditorComponentsMain(object):
    """Holds tests for Atom components."""

    @pytest.mark.test_case_id("C32078118")  # Decal
    @pytest.mark.test_case_id("C32078119")  # DepthOfField
    @pytest.mark.test_case_id("C32078120")  # Directional Light
    @pytest.mark.test_case_id("C32078121")  # Exposure Control
    @pytest.mark.test_case_id("C32078115")  # Global Skylight (IBL)
    @pytest.mark.test_case_id("C32078125")  # Physical Sky
    @pytest.mark.test_case_id("C32078127")  # PostFX Layer
    @pytest.mark.test_case_id("C32078131")  # PostFX Radius Weight Modifier
    @pytest.mark.test_case_id("C32078117")  # Light
    @pytest.mark.test_case_id("C36525660")  # Display Mapper
    def test_AtomEditorComponents_AddedToEntity(self, request, editor, level, workspace, project, launcher_platform):
        """
        Please review the hydra script run by this test for more specific test info.
        Tests the Atom components & verifies all "expected_lines" appear in Editor.log
        """
        cfg_args = [level]

        expected_lines = [
            # Decal Component
            "Decal Entity successfully created",
            "Decal_test: Component added to the entity: True",
            "Decal_test: Component removed after UNDO: True",
            "Decal_test: Component added after REDO: True",
            "Decal_test: Entered game mode: True",
            "Decal_test: Exit game mode: True",
            "Decal Controller|Configuration|Material: SUCCESS",
            "Decal_test: Entity is hidden: True",
            "Decal_test: Entity is shown: True",
            "Decal_test: Entity deleted: True",
            "Decal_test: UNDO entity deletion works: True",
            "Decal_test: REDO entity deletion works: True",
            # DepthOfField Component
            "DepthOfField Entity successfully created",
            "DepthOfField_test: Component added to the entity: True",
            "DepthOfField_test: Component removed after UNDO: True",
            "DepthOfField_test: Component added after REDO: True",
            "DepthOfField_test: Entered game mode: True",
            "DepthOfField_test: Exit game mode: True",
            "DepthOfField_test: Entity disabled initially: True",
            "DepthOfField_test: Entity enabled after adding required components: True",
            "DepthOfField Controller|Configuration|Camera Entity: SUCCESS",
            "DepthOfField_test: Entity is hidden: True",
            "DepthOfField_test: Entity is shown: True",
            "DepthOfField_test: Entity deleted: True",
            "DepthOfField_test: UNDO entity deletion works: True",
            "DepthOfField_test: REDO entity deletion works: True",
            # Directional Light Component
            "Directional Light Entity successfully created",
            "Directional Light_test: Component added to the entity: True",
            "Directional Light_test: Component removed after UNDO: True",
            "Directional Light_test: Component added after REDO: True",
            "Directional Light_test: Entered game mode: True",
            "Directional Light_test: Exit game mode: True",
            "Directional Light_test: Entity is hidden: True",
            "Directional Light_test: Entity is shown: True",
            "Directional Light_test: Entity deleted: True",
            "Directional Light_test: UNDO entity deletion works: True",
            "Directional Light_test: REDO entity deletion works: True",
            # Exposure Control Component
            "Exposure Control Entity successfully created",
            "Exposure Control_test: Component added to the entity: True",
            "Exposure Control_test: Component removed after UNDO: True",
            "Exposure Control_test: Component added after REDO: True",
            "Exposure Control_test: Entered game mode: True",
            "Exposure Control_test: Exit game mode: True",
            "Exposure Control_test: Entity disabled initially: True",
            "Exposure Control_test: Entity enabled after adding required components: True",
            "Exposure Control_test: Entity is hidden: True",
            "Exposure Control_test: Entity is shown: True",
            "Exposure Control_test: Entity deleted: True",
            "Exposure Control_test: UNDO entity deletion works: True",
            "Exposure Control_test: REDO entity deletion works: True",
            # Global Skylight (IBL) Component
            "Global Skylight (IBL) Entity successfully created",
            "Global Skylight (IBL)_test: Component added to the entity: True",
            "Global Skylight (IBL)_test: Component removed after UNDO: True",
            "Global Skylight (IBL)_test: Component added after REDO: True",
            "Global Skylight (IBL)_test: Entered game mode: True",
            "Global Skylight (IBL)_test: Exit game mode: True",
            "Global Skylight (IBL) Controller|Configuration|Diffuse Image: SUCCESS",
            "Global Skylight (IBL) Controller|Configuration|Specular Image: SUCCESS",
            "Global Skylight (IBL)_test: Entity is hidden: True",
            "Global Skylight (IBL)_test: Entity is shown: True",
            "Global Skylight (IBL)_test: Entity deleted: True",
            "Global Skylight (IBL)_test: UNDO entity deletion works: True",
            "Global Skylight (IBL)_test: REDO entity deletion works: True",
            # Physical Sky Component
            "Physical Sky Entity successfully created",
            "Physical Sky component was added to entity",
            "Entity has a Physical Sky component",
            "Physical Sky_test: Component added to the entity: True",
            "Physical Sky_test: Component removed after UNDO: True",
            "Physical Sky_test: Component added after REDO: True",
            "Physical Sky_test: Entered game mode: True",
            "Physical Sky_test: Exit game mode: True",
            "Physical Sky_test: Entity is hidden: True",
            "Physical Sky_test: Entity is shown: True",
            "Physical Sky_test: Entity deleted: True",
            "Physical Sky_test: UNDO entity deletion works: True",
            "Physical Sky_test: REDO entity deletion works: True",
            # PostFX Layer Component
            "PostFX Layer Entity successfully created",
            "PostFX Layer_test: Component added to the entity: True",
            "PostFX Layer_test: Component removed after UNDO: True",
            "PostFX Layer_test: Component added after REDO: True",
            "PostFX Layer_test: Entered game mode: True",
            "PostFX Layer_test: Exit game mode: True",
            "PostFX Layer_test: Entity is hidden: True",
            "PostFX Layer_test: Entity is shown: True",
            "PostFX Layer_test: Entity deleted: True",
            "PostFX Layer_test: UNDO entity deletion works: True",
            "PostFX Layer_test: REDO entity deletion works: True",
            # PostFX Radius Weight Modifier Component
            "PostFX Radius Weight Modifier Entity successfully created",
            "PostFX Radius Weight Modifier_test: Component added to the entity: True",
            "PostFX Radius Weight Modifier_test: Component removed after UNDO: True",
            "PostFX Radius Weight Modifier_test: Component added after REDO: True",
            "PostFX Radius Weight Modifier_test: Entered game mode: True",
            "PostFX Radius Weight Modifier_test: Exit game mode: True",
            "PostFX Radius Weight Modifier_test: Entity is hidden: True",
            "PostFX Radius Weight Modifier_test: Entity is shown: True",
            "PostFX Radius Weight Modifier_test: Entity deleted: True",
            "PostFX Radius Weight Modifier_test: UNDO entity deletion works: True",
            "PostFX Radius Weight Modifier_test: REDO entity deletion works: True",
            # Light Component
            "Light Entity successfully created",
            "Light_test: Component added to the entity: True",
            "Light_test: Component removed after UNDO: True",
            "Light_test: Component added after REDO: True",
            "Light_test: Entered game mode: True",
            "Light_test: Exit game mode: True",
            "Light_test: Entity is hidden: True",
            "Light_test: Entity is shown: True",
            "Light_test: Entity deleted: True",
            "Light_test: UNDO entity deletion works: True",
            "Light_test: REDO entity deletion works: True",
            # Display Mapper Component
            "Display Mapper Entity successfully created",
            "Display Mapper_test: Component added to the entity: True",
            "Display Mapper_test: Component removed after UNDO: True",
            "Display Mapper_test: Component added after REDO: True",
            "Display Mapper_test: Entered game mode: True",
            "Display Mapper_test: Exit game mode: True",
            "Display Mapper_test: Entity is hidden: True",
            "Display Mapper_test: Entity is shown: True",
            "Display Mapper_test: Entity deleted: True",
            "Display Mapper_test: UNDO entity deletion works: True",
            "Display Mapper_test: REDO entity deletion works: True",
        ]

        unexpected_lines = [
            "Trace::Assert",
            "Trace::Error",
            "Traceback (most recent call last):",
        ]

        hydra.launch_and_validate_results(
            request,
            TEST_DIRECTORY,
            editor,
            "hydra_AtomEditorComponents_AddedToEntity.py",
            timeout=120,
            expected_lines=expected_lines,
            unexpected_lines=unexpected_lines,
            halt_on_unexpected=True,
            null_renderer=True,
            cfg_args=cfg_args,
        )

    @pytest.mark.test_case_id("C34525095")
    def test_AtomEditorComponents_LightComponent(
            self, request, editor, workspace, project, launcher_platform, level):
        """
        Please review the hydra script run by this test for more specific test info.
        Tests that the Light component has the expected property options available to it.
        """
        cfg_args = [level]

        expected_lines = [
            "light_entity Entity successfully created",
            "Entity has a Light component",
            "light_entity_test: Component added to the entity: True",
            f"light_entity_test: Property value is {LIGHT_TYPES['sphere']} which matches {LIGHT_TYPES['sphere']}",
            "Controller|Configuration|Shadows|Enable shadow set to True",
            "light_entity Controller|Configuration|Shadows|Shadowmap size: SUCCESS",
            "Controller|Configuration|Shadows|Shadow filter method set to 1",  # PCF
            "Controller|Configuration|Shadows|Filtering sample count set to 4",
            "Controller|Configuration|Shadows|Filtering sample count set to 64",
            "Controller|Configuration|Shadows|Shadow filter method set to 2",  # ESM
            "Controller|Configuration|Shadows|ESM exponent set to 50.0",
            "Controller|Configuration|Shadows|ESM exponent set to 5000.0",
            "Controller|Configuration|Shadows|Shadow filter method set to 3",  # ESM+PCF
            f"light_entity_test: Property value is {LIGHT_TYPES['spot_disk']} which matches {LIGHT_TYPES['spot_disk']}",
            f"light_entity_test: Property value is {LIGHT_TYPES['capsule']} which matches {LIGHT_TYPES['capsule']}",
            f"light_entity_test: Property value is {LIGHT_TYPES['quad']} which matches {LIGHT_TYPES['quad']}",
            "light_entity Controller|Configuration|Fast approximation: SUCCESS",
            "light_entity Controller|Configuration|Both directions: SUCCESS",
            f"light_entity_test: Property value is {LIGHT_TYPES['polygon']} which matches {LIGHT_TYPES['polygon']}",
            f"light_entity_test: Property value is {LIGHT_TYPES['simple_point']} "
            f"which matches {LIGHT_TYPES['simple_point']}",
            "Controller|Configuration|Attenuation radius|Mode set to 0",
            "Controller|Configuration|Attenuation radius|Radius set to 100.0",
            f"light_entity_test: Property value is {LIGHT_TYPES['simple_spot']} "
            f"which matches {LIGHT_TYPES['simple_spot']}",
            "Controller|Configuration|Shutters|Outer angle set to 45.0",
            "Controller|Configuration|Shutters|Outer angle set to 90.0",
            "light_entity_test: Component added to the entity: True",
            "Light component test (non-GPU) completed.",
        ]

        unexpected_lines = [
            "Trace::Assert",
            "Trace::Error",
            "Traceback (most recent call last):",
        ]

        hydra.launch_and_validate_results(
            request,
            TEST_DIRECTORY,
            editor,
            "hydra_AtomEditorComponents_LightComponent.py",
            timeout=120,
            expected_lines=expected_lines,
            unexpected_lines=unexpected_lines,
            halt_on_unexpected=True,
            null_renderer=True,
            cfg_args=cfg_args,
        )


