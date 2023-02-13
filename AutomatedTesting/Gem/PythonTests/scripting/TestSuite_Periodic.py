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

    """
    od3e/o3de#13481
    This test fails in multi test. QCheckbox state change does not trigger table changes like in hydra/editor test run
    def test_ScriptEvent_AddRemoveMethod_UpdatesInSC(self, request, workspace, editor, launcher_platform):
        from . import ScriptEvent_AddRemoveMethod_UpdatesInSC as test_module
        self._run_test(request, workspace, editor, test_module)
    """


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
