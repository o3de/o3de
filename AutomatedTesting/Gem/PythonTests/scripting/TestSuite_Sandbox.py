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


@pytest.mark.SUITE_sandbox
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomation(TestAutomationBase):

    def test_Opening_Closing_Pane(self, request, workspace, editor, launcher_platform):
        from . import Opening_Closing_Pane as test_module
        self._run_test(request, workspace, editor, test_module)
        
    @pytest.mark.parametrize("level", ["tmp_level"])
    def test_ScriptCanvas_TwoComponents_InteractSuccessfully(self, request, workspace, editor, launcher_platform, level):
        def teardown():
            file_system.delete([os.path.join(workspace.paths.project(), "Levels", level)], True, True)
        request.addfinalizer(teardown)
        file_system.delete([os.path.join(workspace.paths.project(), "Levels", level)], True, True)
        from . import ScriptCanvas_TwoComponents_InteractSuccessfully as test_module
        self._run_test(request, workspace, editor, test_module)

    @pytest.mark.parametrize("level", ["tmp_level"])
    def test_ScriptCanvas_ChangingAssets_ComponentStable(self, request, workspace, editor, launcher_platform, project, level):
        def teardown():
            file_system.delete([os.path.join(workspace.paths.project(), "Levels", level)], True, True)
        request.addfinalizer(teardown)
        file_system.delete([os.path.join(workspace.paths.project(), "Levels", level)], True, True)
        from . import ScriptCanvas_ChangingAssets_ComponentStable as test_module
        self._run_test(request, workspace, editor, test_module)
        
    @pytest.mark.parametrize("level", ["tmp_level"])
    def test_ScriptCanvasComponent_OnEntityActivatedDeactivated_PrintMessage(self, request, workspace, editor, launcher_platform, project, level):
        def teardown():
            file_system.delete([os.path.join(workspace.paths.project(), "Levels", level)], True, True)
        request.addfinalizer(teardown)
        file_system.delete([os.path.join(workspace.paths.project(), "Levels", level)], True, True)
        from . import ScriptCanvasComponent_OnEntityActivatedDeactivated_PrintMessage as test_module
        self._run_test(request, workspace, editor, test_module)

    @pytest.mark.parametrize("level", ["tmp_level"])
    def test_ScriptCanvas_TwoEntities_UseSimultaneously(self, request, workspace, editor, launcher_platform, project, level):
        def teardown():
            file_system.delete([os.path.join(workspace.paths.project(), "Levels", level)], True, True)
        request.addfinalizer(teardown)
        file_system.delete([os.path.join(workspace.paths.project(), "Levels", level)], True, True)
        from . import ScriptCanvas_TwoEntities_UseSimultaneously as test_module
        self._run_test(request, workspace, editor, test_module)

    @pytest.mark.parametrize("level", ["tmp_level"])
    def test_ScriptEvents_HappyPath_SendReceiveAcrossMultiple(self, request, workspace, editor, launcher_platform, project, level):
        def teardown():
            file_system.delete([os.path.join(workspace.paths.project(), "Levels", level)], True, True)
        request.addfinalizer(teardown)
        file_system.delete([os.path.join(workspace.paths.project(), "Levels", level)], True, True)
        from . import ScriptEvents_HappyPath_SendReceiveAcrossMultiple as test_module
        self._run_test(request, workspace, editor, test_module)

    @pytest.mark.parametrize("level", ["tmp_level"])
    def test_ScriptEvents_Default_SendReceiveSuccessfully(self, request, workspace, editor, launcher_platform, project, level):
        def teardown():
            file_system.delete([os.path.join(workspace.paths.project(), "Levels", level)], True, True)
        request.addfinalizer(teardown)
        file_system.delete([os.path.join(workspace.paths.project(), "Levels", level)], True, True)
        from . import ScriptEvents_Default_SendReceiveSuccessfully as test_module
        self._run_test(request, workspace, editor, test_module)

    @pytest.mark.parametrize("level", ["tmp_level"])
    def test_ScriptEvents_ReturnSetType_Successfully(self, request, workspace, editor, launcher_platform, project, level):
        def teardown():
            file_system.delete([os.path.join(workspace.paths.project(), "Levels", level)], True, True)
        request.addfinalizer(teardown)
        file_system.delete([os.path.join(workspace.paths.project(), "Levels", level)], True, True)
        from . import ScriptEvents_ReturnSetType_Successfully as test_module
        self._run_test(request, workspace, editor, test_module)

# NOTE: We had to use hydra_test_utils.py, as TestAutomationBase run_test method
# fails because of pyside_utils import
@pytest.mark.SUITE_sandbox
@pytest.mark.parametrize("launcher_platform", ["windows_editor"])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestScriptCanvasTests(object):
    """
    The following tests use hydra_test_utils.py to launch the editor and validate the results.
    """

    def test_FileMenu_Default_NewAndOpen(self, request, editor, launcher_platform):
        expected_lines = [
            "File->New action working as expected: True",
            "File->Open action working as expected: True",
        ]
        hydra.launch_and_validate_results(
            request, TEST_DIRECTORY, editor, "FileMenu_Default_NewAndOpen.py", expected_lines, auto_test_mode=False, timeout=60,
        )

    def test_ScriptEvent_AddRemoveMethod_UpdatesInSC(self, request, workspace, editor, launcher_platform):
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
            "Success: Initial Child Event created",
            "Success: Second Child Event created",
            "Success: Script event file saved",
            "Success: Method added to scriptevent file",
            "Success: Method removed from scriptevent file",
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
        