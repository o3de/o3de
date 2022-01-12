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
from Atom.atom_utils.atom_constants import LIGHT_TYPES

logger = logging.getLogger(__name__)
TEST_DIRECTORY = os.path.join(os.path.dirname(__file__), "tests")


@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("level", ["auto_test"])
class TestAtomEditorComponentsMain(object):
    """Holds tests for Atom components."""

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

        unexpected_lines = ["Traceback (most recent call last):"]

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
            enable_prefab_system=False,
        )


@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
class TestAutomation(EditorTestSuite):

    enable_prefab_system = False

    #this test is intermittently timing out without ever having executed. sandboxing while we investigate cause.
    @pytest.mark.test_case_id("C36525660")
    class AtomEditorComponents_DisplayMapperAdded(EditorSharedTest):
        from Atom.tests import hydra_AtomEditorComponents_DisplayMapperAdded as test_module

    # this test causes editor to crash when using slices. once automation transitions to prefabs it should pass
    @pytest.mark.test_case_id("C36529666")
    class AtomEditorComponentsLevel_DiffuseGlobalIlluminationAdded(EditorSharedTest):
        from Atom.tests import hydra_AtomEditorComponentsLevel_DiffuseGlobalIlluminationAdded as test_module

    # this test causes editor to crash when using slices. once automation transitions to prefabs it should pass
    @pytest.mark.test_case_id("C36525660")
    class AtomEditorComponentsLevel_DisplayMapperAdded(EditorSharedTest):
        from Atom.tests import hydra_AtomEditorComponentsLevel_DisplayMapperAdded as test_module
