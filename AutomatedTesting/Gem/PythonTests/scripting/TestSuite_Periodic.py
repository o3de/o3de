"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import ly_test_tools
import pytest
import os
import sys
sys.path.append(os.path.dirname(__file__))

import ImportPathHelper as imports
imports.init()

import hydra_test_utils as hydra
import ly_test_tools.environment.file_system as file_system
from ly_test_tools.o3de.editor_test import EditorBatchedTest, EditorTestSuite
from ly_test_tools import LAUNCHERS
from base import TestAutomationBase

TEST_DIRECTORY = os.path.dirname(__file__)


# Bat
@pytest.mark.REQUIRES_gpu
@pytest.mark.SUITE_periodic
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestScriptCanvas(EditorTestSuite):

    class test_NodePalette_HappyPath_CanSelectNode(EditorBatchedTest):
        import NodePalette_HappyPath_CanSelectNode as test_module

    @pytest.mark.xfail(reason="keyboard shortcuts are not behaving properly: https://github.com/o3de/o3de/issues/14802")
    class test_EditMenu_Default_UndoRedo(EditorBatchedTest):
        import EditMenu_Default_UndoRedo as test_module


@pytest.mark.REQUIRES_gpu
@pytest.mark.SUITE_periodic
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomationQtPyTests(TestAutomationBase):

    # Enable only -autotest_mode for these tests. Tests cannot run in -BatchMode due to UI interactions
    global_extra_cmdline_args = []

    def test_ScriptCanvas_ChangingAssets_ComponentStable(self, request, workspace, editor, launcher_platform):
        from . import ScriptCanvas_ChangingAssets_ComponentStable as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_VariableManager_UnpinVariableType_Works(self, request, workspace, editor, launcher_platform):
        from . import VariableManager_UnpinVariableType_Works as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_ScriptCanvas_TwoComponents_InteractSuccessfully(self, request, workspace, editor, launcher_platform):
        from . import ScriptCanvas_TwoComponents_InteractSuccessfully as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_ScriptCanvas_TwoEntities_UseSimultaneously(self, request, workspace, editor, launcher_platform):
        from . import ScriptCanvas_TwoComponents_InteractSuccessfully as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_ScriptCanvasComponent_OnEntityActivatedDeactivated_PrintMessage(self, request, workspace, editor, launcher_platform):
        from . import ScriptCanvasComponent_OnEntityActivatedDeactivated_PrintMessage as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_ScriptEvents_Default_SendReceiveSuccessfully(self, request, workspace, editor, launcher_platform):
        from . import ScriptEvents_Default_SendReceiveSuccessfully as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_ScriptEvents_HappyPath_SendReceiveAcrossMultiple(self, request, workspace, editor, launcher_platform):
        from . import ScriptEvents_HappyPath_SendReceiveAcrossMultiple as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_ScriptEvents_ReturnSetType_Successfully(self, request, workspace, editor, launcher_platform):
        from . import ScriptEvents_ReturnSetType_Successfully as test_module
        self._run_test(request, workspace, editor, test_module)

    @pytest.mark.skip(reason="a Qt checkbox does not trigger: https://github.com/o3de/o3de/issues/14860")
    def test_ScriptEvent_AddRemoveParameter_ActionsSuccessful(self, request, workspace, editor, launcher_platform):
        from . import ScriptEvent_AddRemoveParameter_ActionsSuccessful as test_module
        self._run_test(request, workspace, editor, test_module)

    @pytest.mark.skip(reason="test needs a way to avoid/dismiss modal save as window.")
    def test_ScriptEvent_AddRemoveMethod_UpdatesInSC(self, request, workspace, editor, launcher_platform):
        from . import ScriptEvent_AddRemoveMethod_UpdatesInSC as test_module
        self._run_test(request, workspace, editor, test_module)

    @pytest.mark.skip(reason="a Qt checkbox does not trigger: https://github.com/o3de/o3de/issues/14860")
    def test_ScriptEvents_AllParamDatatypes_CreationSuccess(self, request, workspace, editor, launcher_platform):
        from . import ScriptEvents_AllParamDatatypes_CreationSuccess as test_module
        self._run_test(request, workspace, editor, test_module)

@pytest.mark.REQUIRES_gpu
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

    def test_NodePalette_HappyPath_ClearSelection(self, request, workspace, editor, launcher_platform, project):
        from . import NodePalette_HappyPath_ClearSelection as test_module
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

    @pytest.mark.skip(reason="Test fails on nightly build builds, it needs to be fixed.")
    @pytest.mark.parametrize("level", ["tmp_level"])
    def test_Debugger_HappyPath_TargetMultipleEntities(self, request, workspace, editor, launcher_platform, project, level):
        def teardown():
            file_system.delete([os.path.join(workspace.paths.project(), "Levels", level)], True, True)
        request.addfinalizer(teardown)
        file_system.delete([os.path.join(workspace.paths.project(), "Levels", level)], True, True)
        from . import Debugger_HappyPath_TargetMultipleEntities as test_module
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


# NOTE: We had to use hydra_test_utils.py, as TestAutomationBase run_test method
# fails because of pyside_utils import
@pytest.mark.SUITE_periodic
@pytest.mark.parametrize("launcher_platform", ["windows_editor"])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestScriptCanvasTests(object):

    """
    The following tests use hydra_test_utils.py to launch the editor and validate the results.
    """

    @pytest.mark.skipif(ly_test_tools.LINUX, reason="investigate and update https://github.com/o3de/o3de/issues/15553")
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
            timeout=60,
        )

    @pytest.mark.xfail(reason="keyboard shortcuts are not behaving properly: https://github.com/o3de/o3de/issues/14802")
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
