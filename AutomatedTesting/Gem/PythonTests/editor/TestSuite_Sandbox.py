"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import pytest

from ly_test_tools.o3de.editor_test import EditorBatchedTest, EditorTestSuite


@pytest.mark.SUITE_sandbox
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomationAutoTestMode(EditorTestSuite):

    # Enable only -autotest_mode for these tests. Tests cannot run in -BatchMode due to UI interactions
    global_extra_cmdline_args = ["-autotest_mode"]

    class test_ComponentCRUD_Add_Delete_Components(EditorBatchedTest):
        from .EditorScripts import ComponentCRUD_Add_Delete_Components as test_module

    class test_Menus_FileMenuOptions_Work(EditorBatchedTest):
        from .EditorScripts import Menus_FileMenuOptions as test_module


@pytest.mark.SUITE_sandbox
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomationNoAutoTestMode(EditorTestSuite):

    # Disable -autotest_mode and -BatchMode. Tests cannot run in -BatchMode due to UI interactions, and these tests
    # interact with modal dialogs
    global_extra_cmdline_args = []

    class test_InputBindings_Add_Remove_Input_Events(EditorBatchedTest):
        from .EditorScripts import InputBindings_Add_Remove_Input_Events as test_module
