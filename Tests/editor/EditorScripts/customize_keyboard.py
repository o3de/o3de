"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

C6321576: Basic Function: Customize Keyboard
https://testrail.agscollab.com/index.php?/cases/view/6321576
"""

import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import azlmbr.legacy.general as general
import Tests.ly_shared.pyside_utils as pyside_utils
from PySide2 import QtWidgets, QtGui, QtTest
from PySide2.QtCore import Qt
from Tests.editor.editor_utils.editor_test_helper import EditorTestHelper


class TestCustomizeKeyboard(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="customize_keyboard: ")

    @pyside_utils.wrap_async
    async def run_test(self):
        """
        Summary:
        Assign any new shortcut keys to menu command.

        Expected Behavior:
        Keyboard customizations can be changed, saved, and function properly.

        Test Steps:
        1) Open the Customize Keyboard dialog and Assign any button combination as new shortcut.
        2) Use the newly assigned hotkey.
        3) Restore the settings to default

        Note:
        - This test file must be called from the Lumberyard Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """

        # 1) Open the Customize Keyboard dialog and Assign any button combination as new shortcut
        editor_window = pyside_utils.get_editor_main_window()
        action = pyside_utils.find_child_by_pattern(
            editor_window, {"text": "Customize &Keyboard...", "type": QtWidgets.QAction}
        )
        pyside_utils.trigger_action_async(action)
        active_modal_widget = await pyside_utils.wait_for_modal_widget()
        if active_modal_widget:
            keyboard_customizer = active_modal_widget.findChild(QtWidgets.QDialog, "CustomizeKeyboardDialog")
            if keyboard_customizer:
                print("Customize keyboard window opened successfully")
            keySequenceEdit = pyside_utils.find_child_by_pattern(keyboard_customizer, "keySequenceEdit")
            assign_button = pyside_utils.find_child_by_pattern(keyboard_customizer, "assignButton")
            categories = keyboard_customizer.findChild(QtWidgets.QComboBox, "categories")
            commandsView = keyboard_customizer.findChild(QtWidgets.QListView, "commandsView")

            # Assign a shortcut to the Tools -> Asset Editor action
            categories.setCurrentText("Tools")
            asset_editor_index = pyside_utils.find_child_by_pattern(commandsView, "Asset Editor")
            commandsView.setCurrentIndex(asset_editor_index)
            keySequenceEdit.setKeySequence(QtGui.QKeySequence(Qt.CTRL + Qt.Key_M))

            assign_button.setEnabled(True)
            if assign_button.isEnabled():
                app = QtWidgets.QApplication.instance()
                pyside_utils.click_button_async(assign_button)

                # We need to handle in case this test had failed previously, the shortcut might
                # already be assigned, in which case there will be a second modal dialog
                # confirmation to overwrite it
                try:
                    await pyside_utils.wait_for_condition(lambda: app.activeModalWidget() and app.activeModalWidget() != active_modal_widget, timeout=0.1)
                    confirmation_popup = app.activeModalWidget()
                    message_box = confirmation_popup.findChild(QtWidgets.QMessageBox)
                    button = message_box.button(QtWidgets.QMessageBox.Yes)
                    pyside_utils.click_button_async(button)
                    await pyside_utils.wait_for_destroyed(confirmation_popup)
                except pyside_utils.EventLoopTimeoutException:
                    # If this timed out, it just means that the shortcut wasn't already assigned
                    pass

            # "Close" button acts as "Save and Close" in this widget
            button_box = pyside_utils.find_child_by_pattern(keyboard_customizer, "buttonBox")
            button = button_box.button(QtWidgets.QDialogButtonBox.Close)
            pyside_utils.click_button_async(button)
            await pyside_utils.wait_for_destroyed(active_modal_widget)

        # 2) Use the newly assigned hotkey.
        general.close_pane("Asset Editor")
        QtTest.QTest.keyPress(editor_window, Qt.Key_M, Qt.ControlModifier)
        success = await pyside_utils.wait_for_condition(lambda: general.is_pane_visible("Asset Editor"))
        if success:
            print("New shortcut works : Asset Editor opened")

            # Close it now since we are going to verify the shortcut no longer works
            # after restoring the default settings
            general.close_pane("Asset Editor")

        # 3) Restore the settings to default
        pyside_utils.trigger_action_async(action)
        active_modal_widget = await pyside_utils.wait_for_modal_widget()
        if active_modal_widget:
            button_box = pyside_utils.find_child_by_pattern(active_modal_widget, "buttonBox")
            restore_defaults = pyside_utils.find_child_by_pattern(button_box, "Restore Defaults")
            pyside_utils.click_button_async(restore_defaults)
            confirmation_modal_widget = await pyside_utils.wait_for_modal_widget()
            if confirmation_modal_widget:
                message_box = confirmation_modal_widget.findChild(QtWidgets.QMessageBox)
                button = message_box.button(QtWidgets.QMessageBox.Yes)
                button.click()

            # "Close" button acts as "Save and Close" in this widget
            close_button = button_box.button(QtWidgets.QDialogButtonBox.Close)
            pyside_utils.click_button_async(close_button)
            await pyside_utils.wait_for_destroyed(active_modal_widget)

        # Verify if setting restored
        QtTest.QTest.keyClick(editor_window, Qt.Key_M, Qt.ControlModifier)
        if not general.is_pane_visible("Asset Editor"):
            print("Default shortcuts restored : Asset Editor stays closed")


test = TestCustomizeKeyboard()
test.run()
