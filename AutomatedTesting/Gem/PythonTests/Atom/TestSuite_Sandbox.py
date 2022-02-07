"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import logging
import os

import pytest

import editor_python_test_tools.hydra_test_utils as hydra

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

        @pytest.mark.test_case_id("C32078128")
        def test_AtomEditorComponents_ReflectionProbeAddedToEntity(
                self, request, editor, level, workspace, project, launcher_platform):
            """
            Please review the hydra script run by this test for more specific test info.
            Tests the following Atom components and verifies all "expected_lines" appear in Editor.log:
            1. Reflection Probe
            """
            cfg_args = [level]

            expected_lines = [
                # Reflection Probe Component
                "Reflection Probe Entity successfully created",
                "Reflection Probe_test: Component added to the entity: True",
                "Reflection Probe_test: Component removed after UNDO: True",
                "Reflection Probe_test: Component added after REDO: True",
                "Reflection Probe_test: Entered game mode: True",
                "Reflection Probe_test: Exit game mode: True",
                "Reflection Probe_test: Entity disabled initially: True",
                "Reflection Probe_test: Entity enabled after adding required components: True",
                "Reflection Probe_test: Cubemap is generated: True",
                "Reflection Probe_test: Entity is hidden: True",
                "Reflection Probe_test: Entity is shown: True",
                "Reflection Probe_test: Entity deleted: True",
                "Reflection Probe_test: UNDO entity deletion works: True",
                "Reflection Probe_test: REDO entity deletion works: True",
            ]

            hydra.launch_and_validate_results(
                request,
                TEST_DIRECTORY,
                editor,
                "hydra_AtomEditorComponents_AddedToEntity.py",
                timeout=120,
                expected_lines=expected_lines,
                unexpected_lines=[],
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
            timeout=120,
            expected_lines=expected_lines,
            unexpected_lines=unexpected_lines,
            halt_on_unexpected=True,
            null_renderer=True,
            log_file_name="MaterialEditor.log",
        )

