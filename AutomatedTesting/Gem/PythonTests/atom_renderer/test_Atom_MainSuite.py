"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Main suite tests for the Atom renderer.
"""
import logging
import os

import pytest

import ly_test_tools.environment.file_system as file_system
import editor_python_test_tools.hydra_test_utils as hydra
from atom_renderer.atom_utils.atom_constants import LIGHT_TYPES

logger = logging.getLogger(__name__)
EDITOR_TIMEOUT = 120
TEST_DIRECTORY = os.path.join(os.path.dirname(__file__), "atom_hydra_scripts")


@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("level", ["auto_test"])
class TestAtomEditorComponentsMain(object):
    """Holds tests for Atom components."""

    def test_AtomEditorComponents_AddedToEntity(self, request, editor, level, workspace, project, launcher_platform):
        """
        Please review the hydra script run by this test for more specific test info.
        Tests the following Atom components and verifies all "expected_lines" appear in Editor.log:
        1. Display Mapper
        2. Light
        3. Radius Weight Modifier
        4. PostFX Layer
        5. Physical Sky
        6. Global Skylight (IBL)
        7. Exposure Control
        8. Directional Light
        9. DepthOfField
        10. Decal (Atom)
        """
        cfg_args = [level]

        expected_lines = [
            # Decal (Atom) Component
            "Decal (Atom) Entity successfully created",
            "Decal (Atom)_test: Component added to the entity: True",
            "Decal (Atom)_test: Component removed after UNDO: True",
            "Decal (Atom)_test: Component added after REDO: True",
            "Decal (Atom)_test: Entered game mode: True",
            "Decal (Atom)_test: Exit game mode: True",
            "Decal (Atom) Controller|Configuration|Material: SUCCESS",
            "Decal (Atom)_test: Entity is hidden: True",
            "Decal (Atom)_test: Entity is shown: True",
            "Decal (Atom)_test: Entity deleted: True",
            "Decal (Atom)_test: UNDO entity deletion works: True",
            "Decal (Atom)_test: REDO entity deletion works: True",
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
            # Radius Weight Modifier Component
            "Radius Weight Modifier Entity successfully created",
            "Radius Weight Modifier_test: Component added to the entity: True",
            "Radius Weight Modifier_test: Component removed after UNDO: True",
            "Radius Weight Modifier_test: Component added after REDO: True",
            "Radius Weight Modifier_test: Entered game mode: True",
            "Radius Weight Modifier_test: Exit game mode: True",
            "Radius Weight Modifier_test: Entity is hidden: True",
            "Radius Weight Modifier_test: Entity is shown: True",
            "Radius Weight Modifier_test: Entity deleted: True",
            "Radius Weight Modifier_test: UNDO entity deletion works: True",
            "Radius Weight Modifier_test: REDO entity deletion works: True",
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
            timeout=EDITOR_TIMEOUT,
            expected_lines=expected_lines,
            unexpected_lines=unexpected_lines,
            halt_on_unexpected=True,
            null_renderer=True,
            cfg_args=cfg_args,
        )

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
            "Controller|Configuration|Shadows|PCF method set to 0",
            "Controller|Configuration|Shadows|PCF method set to 1",
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
            timeout=EDITOR_TIMEOUT,
            expected_lines=expected_lines,
            unexpected_lines=unexpected_lines,
            halt_on_unexpected=True,
            null_renderer=True,
            cfg_args=cfg_args,
        )


@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("launcher_platform", ['windows_generic'])
@pytest.mark.system
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
            # "Trace::Assert",
            # "Trace::Error",
            "Traceback (most recent call last):"
        ]

        hydra.launch_and_validate_results(
            request,
            TEST_DIRECTORY,
            generic_launcher,
            "hydra_AtomMaterialEditor_BasicTests.py",
            run_python="--runpython",
            timeout=80,
            expected_lines=expected_lines,
            unexpected_lines=unexpected_lines,
            halt_on_unexpected=True,
            log_file_name="MaterialEditor.log",
        )
