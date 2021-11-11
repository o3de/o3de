"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT


Test should run in both gpu and non gpu
"""

import pytest
import os
from automatedtesting_shared.base import TestAutomationBase
import ly_test_tools.environment.file_system as file_system


@pytest.mark.SUITE_smoke
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

        self._run_test(request, workspace, editor, test_module, extra_cmdline_args=["--regset=/Amazon/Preferences/EnablePrefabSystem=false"])
