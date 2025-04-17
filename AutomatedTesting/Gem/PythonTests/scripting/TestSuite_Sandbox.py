"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import pytest
import os
import sys

from ly_test_tools import LAUNCHERS

sys.path.append(os.path.dirname(os.path.abspath(__file__)) + '/../automatedtesting_shared')

from base import TestAutomationBase
import ly_test_tools.environment.file_system as file_system
import hydra_test_utils as hydra

TEST_DIRECTORY = os.path.dirname(__file__)


@pytest.mark.SUITE_sandbox
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomation(TestAutomationBase):

    def test_Opening_Closing_Pane(self, request, workspace, editor, launcher_platform):
        from . import Opening_Closing_Pane as test_module
        self._run_test(request, workspace, editor, test_module)


@pytest.mark.REQUIRES_gpu
@pytest.mark.SUITE_sandbox
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomationQtPyTests(TestAutomationBase):

    def test_VariableManager_ExposeVarsToComponent(self, request, workspace, editor, launcher_platform):
        from . import VariableManager_ExposeVarsToComponent as test_module
        self._run_test(request, workspace, editor, test_module)


@pytest.mark.REQUIRES_gpu
@pytest.mark.SUITE_sandbox
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomation(TestAutomationBase):

    def test_ScriptEvent_HappyPath_CreatedWithoutError(self, request, workspace, editor, launcher_platform, project):
        def teardown():
            file_system.delete(
                [os.path.join(workspace.paths.project(), "ScriptCanvas", "test_file.scriptevent")], True, True
            )
        request.addfinalizer(teardown)
        file_system.delete(
            [os.path.join(workspace.paths.project(), "ScriptCanvas", "test_file.scriptevent")], True, True
        )
        from . import ScriptEvent_HappyPath_CreatedWithoutError as test_module
        self._run_test(request, workspace, editor, test_module)


@pytest.mark.SUITE_sandbox
@pytest.mark.parametrize("launcher_platform", ["windows_editor"])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestScriptCanvasTests(object):

    """
    The following tests use hydra_test_utils.py to launch the editor and validate the results.
    """

    def test_ScriptEvent_AddRemoveMethod_UpdatesInSC(self, request, workspace, editor, launcher_platform):
        expected_lines = [
            "Node found in Node Palette",
            "Method removed from scriptevent file",
        ]
        hydra.launch_and_validate_results(
            request,
            TEST_DIRECTORY,
            editor,
            "ScriptEvent_AddRemoveMethod_UpdatesInSC.py",
            expected_lines,
            auto_test_mode=False,
            timeout=60,
        )

    def test_NodeCategory_ExpandOnClick(self, request, editor, launcher_platform):
        expected_lines = [
            "Script Canvas pane successfully opened",
            "Category expanded on left click",
            "Category collapsed on left click",
            "Category expanded on double click",
            "Category collapsed on double click",
        ]
        hydra.launch_and_validate_results(
            request,
            TEST_DIRECTORY,
            editor,
            "NodeCategory_ExpandOnClick.py",
            expected_lines,
            auto_test_mode=False,
            timeout=60,
        )

    def test_Node_HappyPath_DuplicateNode(self, request, editor, launcher_platform):
        expected_lines = [
            "Successfully duplicated node",
        ]
        hydra.launch_and_validate_results(
            request,
            TEST_DIRECTORY,
            editor,
            "Node_HappyPath_DuplicateNode.py",
            expected_lines,
            auto_test_mode=False,
            timeout=60,
        )

    def test_ScriptEvent_AddRemoveParameter_ActionsSuccessful(self, request, editor, launcher_platform):
        expected_lines = [
            "Success: New Script Event created",
            "Success: Child Event created",
            "Success: Successfully saved event asset",
            "Success: Successfully added parameter",
            "Success: Successfully removed parameter",
        ]
        hydra.launch_and_validate_results(
            request,
            TEST_DIRECTORY,
            editor,
            "ScriptEvent_AddRemoveParameter_ActionsSuccessful.py",
            expected_lines,
            auto_test_mode=False,
            timeout=60,
        )

    def test_ScriptEvents_AllParamDatatypes_CreationSuccess(self, request, workspace, editor, launcher_platform):
        def teardown():
            file_system.delete(
                [os.path.join(workspace.paths.project(), "TestAssets", "test_file.scriptevents")], True, True
            )
        request.addfinalizer(teardown)
        file_system.delete(
            [os.path.join(workspace.paths.project(), "TestAssets", "test_file.scriptevents")], True, True
        )
        expected_lines = [
            "Success: New Script Event created",
            "Success: Child Event created",
            "Success: New parameters added",
            "Success: Script event file saved",
            "Success: Node found in Script Canvas",
        ]
        hydra.launch_and_validate_results(
            request,
            TEST_DIRECTORY,
            editor,
            "ScriptEvents_AllParamDatatypes_CreationSuccess.py",
            expected_lines,
            auto_test_mode=False,
            timeout=60,
        )

    def test_FileMenu_Default_NewAndOpen(self, request, editor, launcher_platform):
        expected_lines = [
            "Verified no tabs open: True",
            "New tab opened successfully: True",
            "Open file window triggered successfully: True"
        ]
        hydra.launch_and_validate_results(
            request,
            TEST_DIRECTORY,
            editor,
            "FileMenu_Default_NewAndOpen.py",
            expected_lines,
            auto_test_mode=False,
            timeout=60,
        )

    def test_NewScriptEventButton_HappyPath_ContainsSCCategory(self, request, editor, launcher_platform):
        expected_lines = [
            "New Script event action found: True",
            "Asset Editor opened: True",
            "Asset Editor created with new asset: True",
            "New Script event created in Asset Editor: True",
        ]
        hydra.launch_and_validate_results(
            request,
            TEST_DIRECTORY,
            editor,
            "NewScriptEventButton_HappyPath_ContainsSCCategory.py",
            expected_lines,
            auto_test_mode=False,
            timeout=60,
        )

    def test_GraphClose_Default_SavePrompt(self, request, editor, launcher_platform):
        expected_lines = [
            "New graph created: True",
            "Save prompt opened as expected: True",
            "Close button worked as expected: True",
        ]
        hydra.launch_and_validate_results(
            request,
            TEST_DIRECTORY,
            editor,
            "GraphClose_Default_SavePrompt.py",
            expected_lines,
            auto_test_mode=False,
            timeout=160,
        )

    def test_VariableManager_Default_CreateDeleteVars(self, request, editor, launcher_platform):
        var_types = ["Boolean", "Color", "EntityId", "Number", "String", "Transform", "Vector2", "Vector3", "Vector4"]
        expected_lines = [f"{var_type} variable is created: True" for var_type in var_types]
        expected_lines.extend([f"{var_type} variable is deleted: True" for var_type in var_types])
        hydra.launch_and_validate_results(
            request,
            TEST_DIRECTORY,
            editor,
            "VariableManager_Default_CreateDeleteVars.py",
            expected_lines,
            auto_test_mode=False,
            timeout=60,
        )

    @pytest.mark.parametrize(
        "config",
        [
            {
                "cfg_args": "before_restart",
                "expected_lines": [
                    "All the test panes are opened: True",
                    "Test pane 1 is closed: True",
                    "Location of test pane 2 changed successfully: True",
                    "Test pane 3 resized successfully: True",
                ],
            },
            {
                "cfg_args": "after_restart",
                "expected_lines": [
                    "Test pane retained its visiblity on Editor restart: True",
                    "Test pane retained its location on Editor restart: True",
                    "Test pane retained its size on Editor restart: True",
                ],
            },
        ],
    )
    def test_Pane_PropertiesChanged_RetainsOnRestart(self, request, editor, config, project, launcher_platform):
        hydra.launch_and_validate_results(
            request,
            TEST_DIRECTORY,
            editor,
            "Pane_PropertiesChanged_RetainsOnRestart.py",
            config.get('expected_lines'),
            cfg_args=[config.get('cfg_args')],
            auto_test_mode=False,
            timeout=60,
        )
