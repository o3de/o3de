"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
import pytest
import sys

import ly_test_tools.environment.file_system as file_system

sys.path.append(os.path.dirname(os.path.abspath(__file__)) + '/../automatedtesting_shared')
from base import TestAutomationBase


@pytest.fixture
def remove_test_level(request, workspace, project):
    file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", "tmp_level")], True, True)

    def teardown():
        file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", "tmp_level")], True, True)

    request.addfinalizer(teardown)


@pytest.mark.SUITE_main
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomation(TestAutomationBase):

    def test_BasicEditorWorkflows_LevelEntityComponentCRUD(self, request, workspace, editor, launcher_platform,
                                                           remove_test_level):
        from .EditorScripts import BasicEditorWorkflows_LevelEntityComponentCRUD as test_module
        self._run_test(request, workspace, editor, test_module, batch_mode=False, autotest_mode=False, enable_prefab_system=False)

    @pytest.mark.REQUIRES_gpu
    def test_BasicEditorWorkflows_GPU_LevelEntityComponentCRUD(self, request, workspace, editor, launcher_platform,
                                                               remove_test_level):
        from .EditorScripts import BasicEditorWorkflows_LevelEntityComponentCRUD as test_module
        self._run_test(request, workspace, editor, test_module, batch_mode=False, autotest_mode=False,
                       use_null_renderer=False, enable_prefab_system=False)

    def test_EntityOutlienr_EntityOrdering(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import EntityOutliner_EntityOrdering as test_module
        self._run_test(
            request, 
            workspace, 
            editor, 
            test_module, 
            batch_mode=False, 
            autotest_mode=True,
        )
