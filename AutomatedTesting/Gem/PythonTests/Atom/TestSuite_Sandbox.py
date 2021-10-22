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


@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("level", ["auto_test"])
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
