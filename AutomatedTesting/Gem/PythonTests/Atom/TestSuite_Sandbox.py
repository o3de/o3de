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


class TestAtomEditorComponentsSandbox(object):

    # It requires at least one test
    def test_Dummy(self, request, editor, level, workspace, project, launcher_platform):
        pass


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
            timeout=120,
            expected_lines=expected_lines,
            unexpected_lines=unexpected_lines,
            halt_on_unexpected=True,
            null_renderer=True,
            log_file_name="MaterialEditor.log",
            enable_prefab_system=False,
        )

