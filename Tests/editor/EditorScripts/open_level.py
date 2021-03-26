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
C6351276: Open a level
https://testrail.agscollab.com/index.php?/cases/view/6351276
"""

import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import azlmbr.legacy.general as general
import Tests.ly_shared.pyside_utils as pyside_utils
from PySide2 import QtWidgets
import azlmbr.editor as editor
import azlmbr.bus as bus
from Tests.editor.editor_utils.editor_test_helper import EditorTestHelper


class TestOpenLevel(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="open_level", args=["level"])

    def run_test(self):
        """
        Summary:
        Open a level and verify if the level is loaded properly.

        Expected Behavior:
        A level is opened and loaded into the editor.

        Test Steps:
        1) Open an existing level
        2) Verify if the level is loaded

        Note:
        - This test file must be called from the Lumberyard Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """
        self.focus_changed = False
        def on_focus_changed(old, new):
            if not self.focus_changed:
                self.focus_changed = True
                active_widget = QtWidgets.QApplication.activeModalWidget()
                open_level_as_dialogue = active_widget.findChild(QtWidgets.QWidget, "LevelFileDialog")
                if open_level_as_dialogue.windowTitle() == "Open a Level":
                    print("Open a Level dialog opened")
                level_name = open_level_as_dialogue.findChild(QtWidgets.QLineEdit, "nameLineEdit")
                level_name.setText(self.args["level"])
                button_box = open_level_as_dialogue.findChild(QtWidgets.QDialogButtonBox, "buttonBox")
                button_box.button(QtWidgets.QDialogButtonBox.Ok).click()

        def on_action_triggered(action_name):
            print(f"{action_name} Action triggered")
        
        # 1) Open an existing level
        try:
            editor_window = pyside_utils.get_editor_main_window()
            app = QtWidgets.QApplication.instance()
            app.focusChanged.connect(on_focus_changed)
            action = pyside_utils.get_action_for_menu_path(editor_window, "File", "Open Level")
            action_triggered = lambda: on_action_triggered("Open Level")
            action.triggered.connect(action_triggered)
            action.trigger()
            action.triggered.disconnect(action_triggered)
        except Exception as e:
            print(e)
        finally:
            app.focusChanged.disconnect(on_focus_changed)

        # 2) Verify if the new level is loaded
        general.idle_wait(2.0)
        if editor.EditorToolsApplicationRequestBus(bus.Broadcast, "GetCurrentLevelName") == "Audio_Sample":
            print("An existing level opened : SUCCESS")
        else:
            print("An existing level opened : FAILED")


test = TestOpenLevel()
test.run()