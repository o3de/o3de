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
    @pytest.mark.test_case_id("C1702834", "C1702823")
    def test_Opening_Closing_Pane(self, request, workspace, editor, launcher_platform):
        from . import Opening_Closing_Pane as test_module
        self._run_test(request, workspace, editor, test_module)

    @pytest.mark.test_case_id("C1702824")
    def test_Docking_Pane(self, request, workspace, editor, launcher_platform):
        from . import Docking_Pane as test_module
        self._run_test(request, workspace, editor, test_module)

    @pytest.mark.test_case_id("C1702829")
    def test_Resizing_Pane(self, request, workspace, editor, launcher_platform):
        from . import Resizing_Pane as test_module
        self._run_test(request, workspace, editor, test_module)

    @pytest.mark.test_case_id("T92563190")
    @pytest.mark.parametrize("level", ["tmp_level"])
    def test_ScriptCanvas_TwoComponents(self, request, workspace, editor, launcher_platform, level):
        def teardown():
            file_system.delete([os.path.join(workspace.paths.project(), "Levels", level)], True, True)
        request.addfinalizer(teardown)
        file_system.delete([os.path.join(workspace.paths.project(), "Levels", level)], True, True)
        from . import ScriptCanvas_TwoComponents as test_module
        self._run_test(request, workspace, editor, test_module)

    @pytest.mark.test_case_id("T92562986")
    @pytest.mark.parametrize("level", ["tmp_level"])
    def test_ScriptCanvas_ChangingAssets(self, request, workspace, editor, launcher_platform, project, level):
        def teardown():
            file_system.delete([os.path.join(workspace.paths.project(), "Levels", level)], True, True)
        request.addfinalizer(teardown)
        file_system.delete([os.path.join(workspace.paths.project(), "Levels", level)], True, True)
        from . import ScriptCanvas_ChangingAssets as test_module
        self._run_test(request, workspace, editor, test_module)

    @pytest.mark.test_case_id("T92569079", "T92569081")
    def test_Graph_ZoomInZoomOut(self, request, workspace, editor, launcher_platform):
        from . import Graph_ZoomInZoomOut as test_module
        self._run_test(request, workspace, editor, test_module)

    @pytest.mark.test_case_id("T92568940")
    def test_NodePalette_SelectNode(self, request, workspace, editor, launcher_platform):
        from . import NodePalette_SelectNode as test_module
        self._run_test(request, workspace, editor, test_module)

    @pytest.mark.test_case_id("T92569253")
    @pytest.mark.test_case_id("T92569254")
    @pytest.mark.parametrize("level", ["tmp_level"])
    def test_OnEntityActivatedDeactivated_PrintMessage(self, request, workspace, editor, launcher_platform, project, level):
        def teardown():
            file_system.delete([os.path.join(workspace.paths.project(), "Levels", level)], True, True)
        request.addfinalizer(teardown)
        file_system.delete([os.path.join(workspace.paths.project(), "Levels", level)], True, True)
        from . import OnEntityActivatedDeactivated_PrintMessage as test_module
        self._run_test(request, workspace, editor, test_module)
    
    @pytest.mark.test_case_id("T92562993")
    def test_NodePalette_ClearSelection(self, request, workspace, editor, launcher_platform, project):
        from . import NodePalette_ClearSelection as test_module
        self._run_test(request, workspace, editor, test_module)

    @pytest.mark.test_case_id("T92563191")
    @pytest.mark.parametrize("level", ["tmp_level"])
    def test_ScriptCanvas_TwoEntities(self, request, workspace, editor, launcher_platform, project, level):
        def teardown():
            file_system.delete([os.path.join(workspace.paths.project(), "Levels", level)], True, True)
        request.addfinalizer(teardown)
        file_system.delete([os.path.join(workspace.paths.project(), "Levels", level)], True, True)
        from . import ScriptCanvas_TwoEntities as test_module
        self._run_test(request, workspace, editor, test_module)

    @pytest.mark.test_case_id("T92569013")
    def test_AssetEditor_CreateScriptEventFile(self, request, workspace, editor, launcher_platform, project):
        def teardown():
            file_system.delete(
                [os.path.join(workspace.paths.project(), "ScriptCanvas", "test_file.scriptevent")], True, True
            )
        request.addfinalizer(teardown)
        file_system.delete(
            [os.path.join(workspace.paths.project(), "ScriptCanvas", "test_file.scriptevent")], True, True
        )
        from . import AssetEditor_CreateScriptEventFile as test_module
        self._run_test(request, workspace, editor, test_module)

    @pytest.mark.test_case_id("T92569165", "T92569167", "T92569168", "T92569170")
    def test_Toggle_ScriptCanvasTools(self, request, workspace, editor, launcher_platform):
        from . import Toggle_ScriptCanvasTools as test_module
        self._run_test(request, workspace, editor, test_module)

    @pytest.mark.test_case_id("T92568982")
    def test_NodeInspector_RenameVariable(self, request, workspace, editor, launcher_platform, project):
        from . import NodeInspector_RenameVariable as test_module
        self._run_test(request, workspace, editor, test_module)
        
    @pytest.mark.test_case_id("T92569137")
    def test_Debugging_TargetMultipleGraphs(self, request, workspace, editor, launcher_platform, project):
        from . import Debugging_TargetMultipleGraphs as test_module
        self._run_test(request, workspace, editor, test_module)

    @pytest.mark.test_case_id("T92568856")
    @pytest.mark.parametrize("level", ["tmp_level"])
    def test_Debugging_TargetMultipleEntities(self, request, workspace, editor, launcher_platform, project, level):
        def teardown():
            file_system.delete([os.path.join(workspace.paths.project(), "Levels", level)], True, True)
        request.addfinalizer(teardown)
        file_system.delete([os.path.join(workspace.paths.project(), "Levels", level)], True, True)
        from . import Debugging_TargetMultipleEntities as test_module
        self._run_test(request, workspace, editor, test_module)

    @pytest.mark.test_case_id("T92569049", "T92569051")
    def test_EditMenu_UndoRedo(self, request, workspace, editor, launcher_platform, project):
        from . import EditMenu_UndoRedo as test_module
        self._run_test(request, workspace, editor, test_module)

    @pytest.mark.test_case_id("C1702825", "C1702831")
    def test_UnDockedPane_CloseSCWindow(self, request, workspace, editor, launcher_platform):
        from . import UnDockedPane_CloseSCWindow as test_module
        self._run_test(request, workspace, editor, test_module)

    @pytest.mark.test_case_id("T92562978")
    @pytest.mark.parametrize("level", ["tmp_level"])
    def test_Entity_AddScriptCanvasComponent(self, request, workspace, editor, launcher_platform, project, level):
        def teardown():
            file_system.delete([os.path.join(workspace.paths.project(), "Levels", level)], True, True)
        request.addfinalizer(teardown)
        file_system.delete([os.path.join(workspace.paths.project(), "Levels", level)], True, True)
        from . import Entity_AddScriptCanvasComponent as test_module
        self._run_test(request, workspace, editor, test_module)

    @pytest.mark.test_case_id("C1702821", "C1702832")
    def test_Pane_RetainOnSCRestart(self, request, workspace, editor, launcher_platform):
        from . import Pane_RetainOnSCRestart as test_module
        self._run_test(request, workspace, editor, test_module)

    @pytest.mark.test_case_id("T92567321")
    @pytest.mark.parametrize("level", ["tmp_level"])
    def test_ScriptEvents_SendReceiveAcrossMultiple(self, request, workspace, editor, launcher_platform, project, level):
        def teardown():
            file_system.delete([os.path.join(workspace.paths.project(), "Levels", level)], True, True)
        request.addfinalizer(teardown)
        file_system.delete([os.path.join(workspace.paths.project(), "Levels", level)], True, True)
        from . import ScriptEvents_SendReceiveAcrossMultiple as test_module
        self._run_test(request, workspace, editor, test_module)

    @pytest.mark.test_case_id("T92567320")
    @pytest.mark.parametrize("level", ["tmp_level"])
    def test_ScriptEvents_SendReceiveSuccessfully(self, request, workspace, editor, launcher_platform, project, level):
        def teardown():
            file_system.delete([os.path.join(workspace.paths.project(), "Levels", level)], True, True)
        request.addfinalizer(teardown)
        file_system.delete([os.path.join(workspace.paths.project(), "Levels", level)], True, True)
        from . import ScriptEvents_SendReceiveSuccessfully as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_NodePalette_SearchText_Deletion(self, request, workspace, editor, launcher_platform):
        from . import NodePalette_SearchText_Deletion as test_module
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

    @pytest.mark.test_case_id("T92569037", "T92569039")
    def test_FileMenu_New_Open(self, request, editor, launcher_platform):
        expected_lines = [
            "File->New action working as expected: True",
            "File->Open action working as expected: True",
        ]
        hydra.launch_and_validate_results(
            request, TEST_DIRECTORY, editor, "FileMenu_New_Open.py", expected_lines, auto_test_mode=False, timeout=60,
        )

    @pytest.mark.test_case_id("T92568942")
    def test_AssetEditor_NewScriptEvent(self, request, editor, launcher_platform):
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
            "AssetEditor_NewScriptEvent.py",
            expected_lines,
            auto_test_mode=False,
            timeout=60,
        )

    @pytest.mark.test_case_id("T92563068", "T92563070")
    def test_GraphClose_SavePrompt(self, request, editor, launcher_platform):
        expected_lines = [
            "New graph created: True",
            "Save prompt opened as expected: True",
            "Close button worked as expected: True",
        ]
        hydra.launch_and_validate_results(
            request,
            TEST_DIRECTORY,
            editor,
            "GraphClose_SavePrompt.py",
            expected_lines,
            auto_test_mode=False,
            timeout=60,
        )

    @pytest.mark.test_case_id("T92564789", "T92568873")
    def test_VariableManager_CreateDeleteVars(self, request, editor, launcher_platform):
        var_types = ["Boolean", "Color", "EntityID", "Number", "String", "Transform", "Vector2", "Vector3", "Vector4"]
        expected_lines = [f"Success: {var_type} variable is created" for var_type in var_types]
        expected_lines.extend([f"Success: {var_type} variable is deleted" for var_type in var_types])
        hydra.launch_and_validate_results(
            request,
            TEST_DIRECTORY,
            editor,
            "VariableManager_CreateDeleteVars.py",
            expected_lines,
            auto_test_mode=False,
            timeout=60,
        )