"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

"""
C6308169: Tool Stability & Workflow: Console
https://testrail.agscollab.com/index.php?/cases/view/6308169
"""

import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import azlmbr.legacy.general as general
import Tests.ly_shared.pyside_utils as pyside_utils
from PySide2 import QtWidgets
from PySide2.QtTest import QTest
from PySide2.QtCore import Qt
from Tests.editor.editor_utils.editor_test_helper import EditorTestHelper


class ToolStabilityConsoleTest(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="tool_stability_console: ", args=["level"])

    def run_test(self):
        """
        Summary:
        Tool Stability & Workflow: Console

        Expected Behavior:
        1) Console can be opened
        2) Commands can be entered in the console
        3) Console can be closed

        Test Steps:
        1) Open level
        2) Open console
        3) Close the console
        4) Reopen console

        Note:
        - This test file must be called from the Lumberyard Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """

        def open_console():
            general.open_pane("Console")
            return general.is_pane_visible("Console")

        # 1) Open level
        self.test_success = self.create_level(
            self.args["level"],
            heightmap_resolution=1024,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=4096,
            use_terrain=True,
        )
        general.idle_wait(3.0)

        # 2) Open console
        editor_window = pyside_utils.get_editor_main_window()
        main_window = editor_window.findChild(QtWidgets.QMainWindow)
        if main_window.findChild(QtWidgets.QDockWidget, "Console") is None:
            print(f"Console opened successfully: {open_console()}")
        console = main_window.findChild(QtWidgets.QDockWidget, "Console")
        console_widget = console.findChild(QtWidgets.QWidget, "Console")
        container = console_widget.findChild(QtWidgets.QWidget, "container2")
        line_edit = container.findChild(QtWidgets.QLineEdit, "lineEdit")
        line_edit.setText("r_GetScreenShot=2")
        QTest.keyClick(line_edit, Qt.Key_Enter, Qt.ControlModifier)
        general.idle_wait(1.0)
        print("Ran the command in console")

        # 3) Close the console
        general.close_pane("Console")
        print(f"Console window closed: {not general.is_pane_visible('Console')}")

        # 4) Reopen console
        print(f"Console reopened successfully: {open_console()}")


test = ToolStabilityConsoleTest()
test.run()
