"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Main suite tests for the Atom renderer.
"""
import logging
import os

import pytest

import editor_python_test_tools.hydra_test_utils as hydra

logger = logging.getLogger(__name__)
EDITOR_TIMEOUT = 300
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
