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
C6351273: Create a new level
https://testrail.agscollab.com/index.php?/cases/view/6351273
"""

import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import azlmbr.legacy.general as general
import azlmbr.editor as editor
import azlmbr.bus as bus
import Tests.ly_shared.pyside_utils as pyside_utils
from PySide2 import QtWidgets
from Tests.editor.editor_utils.editor_test_helper import EditorTestHelper


class TestCreateNewLevel(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="create_new_level: ", args=["level"])

    def run_test(self):
        """
        Summary:
        Create a new level and verify if the level is loaded properly.

        Expected Behavior:
        A new level is created and loaded into the editor.

        Test Steps:
        1) Create new level
        2) Verify if the new level is loaded

        Note:
        - This test file must be called from the Lumberyard Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """

        def on_focus_changed(old, new):
            print("focus Changed")
            active_modal_widget = QtWidgets.QApplication.activeModalWidget()
            if active_modal_widget:
                new_level_dlg = active_modal_widget.findChild(QtWidgets.QWidget, "CNewLevelDialog")
                if new_level_dlg:
                    if new_level_dlg.windowTitle() == "New Level":
                        print("New Level dialog opened")
                    grp_box = new_level_dlg.findChild(QtWidgets.QGroupBox, "STATIC_GROUP1")
                    level_name = grp_box.findChild(QtWidgets.QLineEdit, "LEVEL")
                    level_name.setText(self.args["level"])
                    level_folders = grp_box.findChild(QtWidgets.QComboBox, "LEVEL_FOLDERS")
                    level_folders.setCurrentText("Levels/")
                    button_box = new_level_dlg.findChild(QtWidgets.QDialogButtonBox, "buttonBox")
                    button_box.button(QtWidgets.QDialogButtonBox.Ok).click()

        def on_action_triggered():
            print("New Level Action triggered")

        try:
            # 1) Create new level
            # Wait 2.0s for editor to load
            general.idle_wait(2.0)
            editor_window = pyside_utils.get_editor_main_window()
            app = QtWidgets.QApplication.instance()
            app.focusChanged.connect(on_focus_changed)
            action = pyside_utils.get_action_for_menu_path(editor_window, "File", "New Level")
            action.triggered.connect(on_action_triggered)
            action.trigger()
            action.triggered.disconnect(on_action_triggered)

            # 2) Verify if the new level is loaded
            general.idle_wait(3.0)
            if editor.EditorToolsApplicationRequestBus(bus.Broadcast, "GetCurrentLevelName") == self.args["level"]:
                print("Create and load new level: SUCCESS")
            else:
                print("Create and load new level: FAILED")

        except Exception as e:
            print(e)
        finally:
            print("Disconnecting focusChanged signal")
            app.focusChanged.disconnect(on_focus_changed)


test = TestCreateNewLevel()
test.run()
