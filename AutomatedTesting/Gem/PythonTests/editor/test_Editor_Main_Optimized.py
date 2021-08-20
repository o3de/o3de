"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
import pytest

import ly_test_tools.environment.file_system as file_system
from ly_test_tools.o3de.editor_test import EditorSingleTest, EditorSharedTest, EditorParallelTest, EditorTestSuite


@pytest.mark.SUITE_main
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomation(EditorTestSuite):

    class test_BasicEditorWorkflows_LevelEntityComponentCRUD(EditorSingleTest):
        # Disable -BatchMode and -autotest_mode
        EditorTestSuite.global_extra_cmdline_args = []

        # Custom teardown to remove slice asset created during test
        def teardown(self, request, workspace, editor, editor_test_results, launcher_platform):
            file_system.delete([os.path.join(workspace.paths.engine_root(), "AutomatedTesting", "Levels", "tmp_level")],
                               True, True)
        from .EditorScripts import BasicEditorWorkflows_LevelEntityComponentCRUD as test_module

    @pytest.mark.REQUIRES_gpu
    class test_BasicEditorWorkflows_GPU_LevelEntityComponentCRUD(EditorSingleTest):
        # Disable -BatchMode, -autotest_mode, and null renderer
        EditorTestSuite.global_extra_cmdline_args = []
        EditorTestSuite.use_null_renderer = False

        # Custom teardown to remove slice asset created during test
        def teardown(self, request, workspace, editor, editor_test_results, launcher_platform):
            file_system.delete([os.path.join(workspace.paths.engine_root(), "AutomatedTesting", "Levels", "tmp_level")],
                               True, True)
        from .EditorScripts import BasicEditorWorkflows_LevelEntityComponentCRUD as test_module
