"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import pytest

from ly_test_tools.o3de.editor_test import EditorSingleTest, EditorSingleTest, EditorParallelTest, EditorTestSuite


@pytest.mark.SUITE_sandbox
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomationAutoTestMode(EditorTestSuite):

    # Enable only -autotest_mode for these tests. Tests cannot run in -BatchMode due to UI interactions
    global_extra_cmdline_args = ["-autotest_mode"]

    class test_ComponentCRUD_Add_Delete_Components(EditorSingleTest):
        from .EditorScripts import ComponentCRUD_Add_Delete_Components as test_module

    class test_Menus_FileMenuOptions_Work(EditorSingleTest):
        from .EditorScripts import Menus_FileMenuOptions as test_module