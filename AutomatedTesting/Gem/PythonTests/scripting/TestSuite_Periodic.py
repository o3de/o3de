"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import pytest
import os
import sys
sys.path.append(os.path.dirname(__file__))

import ImportPathHelper as imports
imports.init()

import hydra_test_utils as hydra
import ly_test_tools.environment.file_system as file_system
from ly_test_tools import LAUNCHERS
from base import TestAutomationBase

TEST_DIRECTORY = os.path.dirname(__file__)


@pytest.mark.SUITE_periodic
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomation(TestAutomationBase):
    def test_Pane_HappyPath_OpenCloseSuccessfully(self, request, workspace, editor, launcher_platform):
        from . import Pane_HappyPath_OpenCloseSuccessfully as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_Pane_HappyPath_DocksProperly(self, request, workspace, editor, launcher_platform):
        from . import Pane_HappyPath_DocksProperly as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_Pane_HappyPath_ResizesProperly(self, request, workspace, editor, launcher_platform):
        from . import Pane_HappyPath_ResizesProperly as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_Graph_HappyPath_ZoomInZoomOut(self, request, workspace, editor, launcher_platform):
        from . import Graph_HappyPath_ZoomInZoomOut as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_NodePalette_HappyPath_CanSelectNode(self, request, workspace, editor, launcher_platform):
        from . import NodePalette_HappyPath_CanSelectNode as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_NodePalette_HappyPath_ClearSelection(self, request, workspace, editor, launcher_platform, project):
        from . import NodePalette_HappyPath_ClearSelection as test_module
        self._run_test(request, workspace, editor, test_module)

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

    def test_ScriptCanvasTools_Toggle_OpenCloseSuccess(self, request, workspace, editor, launcher_platform):
        from . import ScriptCanvasTools_Toggle_OpenCloseSuccess as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_NodeInspector_HappyPath_VariableRenames(self, request, workspace, editor, launcher_platform, project):
        from . import NodeInspector_HappyPath_VariableRenames as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_Debugger_HappyPath_TargetMultipleGraphs(self, request, workspace, editor, launcher_platform, project):
        from . import Debugger_HappyPath_TargetMultipleGraphs as test_module
        self._run_test(request, workspace, editor, test_module)

    @pytest.mark.parametrize("level", ["tmp_level"])
    def test_Debugger_HappyPath_TargetMultipleEntities(self, request, workspace, editor, launcher_platform, project, level):
        def teardown():
            file_system.delete([os.path.join(workspace.paths.project(), "Levels", level)], True, True)
        request.addfinalizer(teardown)
        file_system.delete([os.path.join(workspace.paths.project(), "Levels", level)], True, True)
        from . import Debugger_HappyPath_TargetMultipleEntities as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_EditMenu_Default_UndoRedo(self, request, workspace, editor, launcher_platform, project):
        from . import EditMenu_Default_UndoRedo as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_Pane_Undocked_ClosesSuccessfully(self, request, workspace, editor, launcher_platform):
        from . import Pane_Undocked_ClosesSuccessfully as test_module
        self._run_test(request, workspace, editor, test_module)

    @pytest.mark.parametrize("level", ["tmp_level"])
    def test_Entity_HappyPath_AddScriptCanvasComponent(self, request, workspace, editor, launcher_platform, project, level):
        def teardown():
            file_system.delete([os.path.join(workspace.paths.project(), "Levels", level)], True, True)
        request.addfinalizer(teardown)
        file_system.delete([os.path.join(workspace.paths.project(), "Levels", level)], True, True)
        from . import Entity_HappyPath_AddScriptCanvasComponent as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_Pane_Default_RetainOnSCRestart(self, request, workspace, editor, launcher_platform):
        from . import Pane_Default_RetainOnSCRestart as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_NodeCategory_ExpandOnClick(self, request, workspace, editor, launcher_platform):
        from . import NodeCategory_ExpandOnClick as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_NodePalette_SearchText_Deletion(self, request, workspace, editor, launcher_platform):
        from . import NodePalette_SearchText_Deletion as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_VariableManager_UnpinVariableType_Works(self, request, workspace, editor, launcher_platform):
        from . import VariableManager_UnpinVariableType_Works as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_Node_HappyPath_DuplicateNode(self, request, workspace, editor, launcher_platform):
        from . import Node_HappyPath_DuplicateNode as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_ScriptEvent_AddRemoveParameter_ActionsSuccessful(self, request, workspace, editor, launcher_platform):
        def teardown():
            file_system.delete(
                [os.path.join(workspace.paths.project(), "ScriptCanvas", "test_file.scriptevent")], True, True
            )
        request.addfinalizer(teardown)
        file_system.delete(
            [os.path.join(workspace.paths.project(), "ScriptCanvas", "test_file.scriptevent")], True, True
        )
        from . import ScriptEvent_AddRemoveParameter_ActionsSuccessful as test_module
        self._run_test(request, workspace, editor, test_module)

# NOTE: We had to use hydra_test_utils.py, as TestAutomationBase run_test method
# fails because of pyside_utils import
@pytest.mark.SUITE_periodic
@pytest.mark.parametrize("launcher_platform", ["windows_editor"])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestScriptCanvasTests(object):
    """
    The following tests use hydra_test_utils.py to launch the editor and validate the results.
    """

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
            timeout=60,
        )

    def test_VariableManager_Default_CreateDeleteVars(self, request, editor, launcher_platform):
        var_types = ["Boolean", "Color", "EntityID", "Number", "String", "Transform", "Vector2", "Vector3", "Vector4"]
        expected_lines = [f"Success: {var_type} variable is created" for var_type in var_types]
        expected_lines.extend([f"Success: {var_type} variable is deleted" for var_type in var_types])
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
        