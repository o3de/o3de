"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
#
# This is a pytest module to test the basic integration of PySide2 (aka Qt for Python)
#
import pytest
import time
import logging
import os
import shutil

from test_tools import WINDOWS_LAUNCHER
import test_tools.shared.log_monitor
import test_tools.launchers.phase
import test_tools.builtin.fixtures as fixtures

# Use the built-in workspace and editor fixtures.
# These will configure the requested project and run the editor.
workspace = fixtures.use_fixture(fixtures.builtin_workspace_fixture, scope='function')
editor = fixtures.use_fixture(fixtures.editor, scope='function')

logger = logging.getLogger(__name__)


@pytest.mark.parametrize("platform,configuration,project,spec", [
    pytest.param("win_x64_vs2017", "profile", "AutomatedTesting", "all", marks=pytest.mark.skipif(not WINDOWS_LAUNCHER, reason="Only supported on Windows hosts")),
])
class TestEditorMenuBarAutomation(object):

    def test_MenuBar(self, request, editor, project):
        logger.debug("Running automated test")

        request.addfinalizer(editor.ensure_stopped)

        editor.deploy()
        editor.launch(["--runpython", "@engroot@/Gems/QtForPython/Code/Tests/pyside_auto_menubar_test_case.py"])

        editorlog_file = os.path.join(editor.workspace.release.paths.project_log(), 'Editor.log')

        expected_lines = [
            "QtForPython Is Ready",
            "Value allWindows greater than zero",
            "GetMainWindowId",
            "Get QtWidgets.QMainWindow",
            "Value menuBar is valid",
            "Found File action",
            "Found Edit action",
            "Found Game action",
            "Found Tools action"
        ]

        test_tools.shared.log_monitor.monitor_for_expected_lines(editor, editorlog_file, expected_lines)

        # Rely on the test script to quit after running
        editor.run(test_tools.launchers.phase.WaitForLauncherToQuit(editor, 10))
