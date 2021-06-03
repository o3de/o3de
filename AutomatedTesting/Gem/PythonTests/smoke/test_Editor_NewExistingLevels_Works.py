"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

import pytest
import os
from automatedtesting_shared.base import TestAutomationBase
import ly_test_tools.environment.file_system as file_system


@pytest.mark.SUITE_sandbox
@pytest.mark.parametrize("launcher_platform", ["windows_editor"])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("level", ["temp_level"])
class TestAutomation(TestAutomationBase):
    def test_Editor_NewExistingLevels_Works(self, request, workspace, editor, level, project, launcher_platform):
        def teardown():
            file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)

        request.addfinalizer(teardown)
        file_system.delete([os.path.join(workspace.paths.engine_root(), project, "Levels", level)], True, True)

        from . import Editor_NewExistingLevels_Works as test_module

        self._run_test(request, workspace, editor, test_module)
