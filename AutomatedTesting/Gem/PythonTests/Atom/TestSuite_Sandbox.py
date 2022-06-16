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

    # this test is intermittently timing out without ever having executed. sandboxing while we investigate cause.
    @pytest.mark.test_case_id("C36525660")
    class AtomEditorComponents_DisplayMapperAdded(EditorSharedTest):
        from Atom.tests import hydra_AtomEditorComponents_DisplayMapperAdded as test_module

    # The "Sponza" level is failing with a hard lock 4-12% of the time, needs root causing and fixing.
    @pytest.mark.test_case_id("C36529679")
    class AtomLevelLoadTest_Editor_Sandbox(EditorSharedTest):
        from Atom.tests import hydra_Atom_LevelLoadTest_Sandbox as test_module

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
            "P1: Asset Browser visibility working as expected: True",
            "P1: Inspector visibility working as expected: True",
        ]
        unexpected_lines = [
            # Including any lines in unexpected_lines will cause the test to run for the duration of the timeout
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
        )
