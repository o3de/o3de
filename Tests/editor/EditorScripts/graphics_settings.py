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
C6321557: Basic Function: Graphics Settings
https://testrail.agscollab.com/index.php?/cases/view/6321557
"""

import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import azlmbr.legacy.general as general

import Tests.ly_shared.pyside_utils as pyside_utils
from PySide2 import QtWidgets, QtTest, QtCore
from Tests.editor.editor_utils.editor_test_helper import EditorTestHelper
from enum import Enum, auto

class FocusChangeState(Enum):
    IGNORE = auto()  # We're not waiting for anything
    SETTINGS_WINDOW_APPEAR = auto()  # Waiting for the settings window to appear
    WARNING_DIALOG_APPEAR = auto()  # Waiting for the warning about risks of changing settings
    POST_WARNING_DIALOG = auto()  # After that, it'll either be a log message or source control warning.
    SAVE_WARNING_APPEAR = auto()  # Waiting for the "Could not save" dialog if source control stopped us.
    LOG_WINDOW_APPEAR = auto()  # Waiting for the log window to appear
    CLOSE_WITHOUT_SAVING = auto()  # Waiting for the close without saving dialog.


class TestGraphicsSettings(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="graphics_settings: ", args=["level"])

    def run_test(self):
        """
        Summary:
        Open the graphics settings, change a value and ensure the setting changes.

        Expected Behavior:
        Graphics settings can be changed.

        Test Steps:
        1) Open the graphics settings dialog.
        2) Check the Save button is disabled.
        3) Change a setting.
        4) Check the save button is enabled.
        5) Press the save button.
        6) Check a warning dialog has appeared.
        7) Press Yes.
        8) Check the dialog has closed.

        Note:
        - This test file must be called from the Lumberyard Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """

        def fail_test(msg):
            print(msg)
            print("Test failed.")
            sys.exit()

        current_state = FocusChangeState.IGNORE
        settings_have_saved = False
        last_window_seen = None

        def handle_settings_dialog():
            nonlocal current_state
            current_state = FocusChangeState.IGNORE

            general.idle_wait(0.0)

            active_widget = QtWidgets.QApplication.activeModalWidget()

            if active_widget is None:
                fail_test("Failed to find graphics settings dialog.")

            save_button = pyside_utils.find_child_by_pattern(active_widget, "Save")
            if save_button is None:
                fail_test("Failed to find Save button")

            if type(save_button) != QtWidgets.QToolButton:
                fail_test("Failed to find Save button 2")

            if save_button.isEnabled():
                fail_test("Save button is enabled before change.")

            spin_box = pyside_utils.find_child_by_pattern(active_widget, {"objectName": "e_viewdistratiocustom"})
            if spin_box is None:
                fail_test("Failed to find spinbox.")

            if type(spin_box) != QtWidgets.QDoubleSpinBox:
                fail_test("Failed to find spinbox.")

            current_value = spin_box.value()
            spin_box.setValue(current_value * 2)

            new_value = spin_box.value()
            if current_value == new_value:
                fail_test("Failed to change value.")

            if not save_button.isEnabled():
                fail_test("Save button is not enabled after change.")

            current_state = FocusChangeState.WARNING_DIALOG_APPEAR
            save_button.click()

            if not settings_have_saved:
                current_state = FocusChangeState.CLOSE_WITHOUT_SAVING

            active_widget.close()

        def handle_warning_dialog():
            nonlocal current_state
            current_state = FocusChangeState.IGNORE
            active_widget = QtWidgets.QApplication.activeModalWidget()

            if active_widget is None:
                fail_test("Failed to find warning dialog.")

            msg_box = active_widget.findChild(QtWidgets.QMessageBox)
            if msg_box is None:
                fail_test("Unable to find message box in warning dialog")

            if not msg_box.text().startswith("A non-tested setting could potentially crash the game"):
                fail_test("Unexpected text in warning dialog: " + msg_box.text())

            yes_button = pyside_utils.find_child_by_pattern(active_widget, "&Yes")
            if yes_button is None:
                fail_test("Failed to find yes button.")

            current_state = FocusChangeState.POST_WARNING_DIALOG
            yes_button.click()

        def press_button_in_dialog(button_text, next_state):
            nonlocal current_state
            current_state = FocusChangeState.IGNORE

            active_widget = QtWidgets.QApplication.activeModalWidget()

            button = pyside_utils.find_child_by_pattern(active_widget, button_text)
            if button is None:
                fail_test("Failed to find " + button_text + " button.")

            current_state = next_state
            button.click()

        def check_for_save_success(log_dialog_widget):
            nonlocal settings_have_saved

            msg_box = log_dialog_widget.findChild(QtWidgets.QMessageBox)
            if msg_box is None:
                return

            if msg_box.text().startswith("Updated the graphics setting correctly"):
                settings_have_saved = True

        def on_focus_changed(old, new):
            nonlocal current_state, settings_have_saved, last_window_seen

            if current_state == FocusChangeState.IGNORE:
                return

            active_widget = QtWidgets.QApplication.activeModalWidget()
            if active_widget is None:
                return

            # Make sure the window has changed.
            if active_widget == last_window_seen:
                return

            last_window_seen = active_widget

            title = pyside_utils.find_child_by_pattern(active_widget, "title")
            if title is None:
                return

            if current_state == FocusChangeState.SETTINGS_WINDOW_APPEAR:
                # We're waiting for the settings dialog, ensure the right one has appeared.
                if title.text() == "Graphics Settings":
                    handle_settings_dialog()
            elif current_state == FocusChangeState.WARNING_DIALOG_APPEAR:
                if title.text() == "Warning":
                    handle_warning_dialog()
            elif current_state == FocusChangeState.POST_WARNING_DIALOG:
                # This could be a source control dialog, or a log window if the settings file exists and is writable.
                if title.text() == "Source Control":
                    # Just use overwrite for the project settings file.
                    press_button_in_dialog("Overwrite", FocusChangeState.POST_WARNING_DIALOG)
                elif title.text() == "Log":
                    # After this, there will be another log window for the file that did save.
                    check_for_save_success(active_widget)
                    press_button_in_dialog("OK", FocusChangeState.LOG_WINDOW_APPEAR)
            elif current_state == FocusChangeState.SAVE_WARNING_APPEAR:
                if title.text() == "Warning":
                    press_button_in_dialog("OK", FocusChangeState.LOG_WINDOW_APPEAR)
            elif current_state == FocusChangeState.LOG_WINDOW_APPEAR:
                if title.text() == "Log":
                    check_for_save_success(active_widget)
                    press_button_in_dialog("OK", FocusChangeState.IGNORE)
            elif current_state == FocusChangeState.CLOSE_WITHOUT_SAVING:
                if title.text() == "Warning":
                    press_button_in_dialog("&Yes", FocusChangeState.IGNORE)

        EditorTestHelper.after_level_load(self)

        app = QtWidgets.QApplication.instance()
        app.focusChanged.connect(on_focus_changed)

        editor_window = pyside_utils.get_editor_main_window()
        action = pyside_utils.get_action_for_menu_path(editor_window, "Edit", "Editor Settings", "Graphics Settings")
        if action is None:
            fail_test("Failed to find Graphics Settings action.")

        current_state = FocusChangeState.SETTINGS_WINDOW_APPEAR
        action.trigger()

        app.focusChanged.disconnect(on_focus_changed)

        if settings_have_saved:
            print("Graphics Settings test successful.")
        else:
            print("Graphics Settings test failed.")


test = TestGraphicsSettings()
test.run()

