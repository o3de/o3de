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
C697735: Custom layouts can be saved
C697736: Custom/Default layouts can be loaded
"""

import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import azlmbr.legacy.general as general
import Tests.ly_shared.pyside_utils as pyside_utils
from PySide2 import QtWidgets
from Tests.editor.editor_utils.editor_test_helper import EditorTestHelper


class TestLoadLayout(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="load_layout")

    def run_test(self):
            """
            Summary:
            Creates a custom Editor layout, resets to the default options, and then reloads the custom layout.

            Expected Behavior:
            Pane/tool layout matches expected results for both default and custom layout.

            Test Steps:
             1) Open Editor
             2) Open a few tools and dock them/leave floating
             3) Save the custom layout
             4) Restore the default layout and verify proper tools/panes are open
             5) Load the custom layout and verify proper tools/panes are open
             6) Delete the custom layout to cleanup

            Note:
            - This test file must be called from the Lumberyard Editor command terminal
            - Any passed and failed tests are written to the Editor.log file.
                    Parsing the file or running a log_monitor are required to observe the test results.

            :return: None
            """
            custom_layout_name = "custom_layout_test"
            editor_window = pyside_utils.get_editor_main_window()
            menu_paths = [
                ("Layouts", "Save Layout"),
                ("Layouts", "Restore Default Layout"),
                ("Layouts", custom_layout_name, "Load"),
                ("Layouts", custom_layout_name, "Delete")
            ]

            def on_save_layout_trigger():
                active_modal_widget = QtWidgets.QApplication.activeModalWidget()
                if active_modal_widget:
                    save_level_as_dialogue = active_modal_widget.findChild(QtWidgets.QInputDialog)
                    if save_level_as_dialogue.windowTitle() == "Layout Name":
                        print("The 'Layout Name' dialog appeared")
                        layout_name_field = active_modal_widget.findChild(QtWidgets.QLineEdit)
                        layout_name_field.setText(custom_layout_name)
                        button_box = active_modal_widget.findChild(QtWidgets.QDialogButtonBox)
                        button_box.button(QtWidgets.QDialogButtonBox.Ok).click()

            def on_restore_default_layout_trigger():
                active_modal_widget = QtWidgets.QApplication.activeModalWidget()
                if active_modal_widget:
                    restore_default_layout_dialog = active_modal_widget.findChild(QtWidgets.QMessageBox)
                    if restore_default_layout_dialog.windowTitle() == "Restore Default Layout":
                        print("The 'Restore Default Layout' dialog appeared")
                        button_box = active_modal_widget.findChildren(QtWidgets.QDialogButtonBox)[0]
                        button_box.button(QtWidgets.QDialogButtonBox.RestoreDefaults).click()

            def on_custom_layout_load_trigger():
                print("Restoring default layout")

            def on_delete_custom_layout_trigger():
                print("Deleting custom layout")
                active_modal_widget = QtWidgets.QApplication.activeModalWidget()
                if active_modal_widget:
                    delete_custom_layout_dialog_button_box = active_modal_widget.findChild(QtWidgets.QDialogButtonBox)
                    if delete_custom_layout_dialog_button_box:
                        print("The 'Delete Layout' dialog appeared")
                        delete_custom_layout_dialog_button_box.button(QtWidgets.QDialogButtonBox.Yes).click()

            def trigger_view_menu_option(menu_path, on_trigger):
                app = QtWidgets.QApplication.instance()
                app.focusChanged.connect(on_trigger)
                try:
                    action = pyside_utils.get_action_for_menu_path(editor_window, "View", *menu_path)
                    action.trigger()
                    print(f"{action.iconText()} Action triggered")
                finally:
                    app.focusChanged.disconnect(on_trigger)

            # Open a few floating tools
            general.open_pane('Landscape Canvas')
            general.open_pane('Track View')
            general.open_pane('UI Editor')

            # Close a few open tools
            general.close_pane('Console')
            general.close_pane('Asset Browser')

            # Verify proper tools are open in the Editor prior to saving layout
            landscape_canvas_open = general.is_pane_visible('Landscape Canvas')
            track_view_open = general.is_pane_visible('Track View')
            ui_editor_open = general.is_pane_visible('UI Editor')
            console_closed = not general.is_pane_visible('Console')
            asset_browser_closed = not general.is_pane_visible('Asset Browser')
            self.test_success = landscape_canvas_open and track_view_open and ui_editor_open and console_closed \
                                and asset_browser_closed
            if self.test_success:
                print('Custom layout setup complete')

            # Save layout
            trigger_view_menu_option(menu_paths[0], on_save_layout_trigger)

            # Trigger Restore Default Layout option and click Restore Defaults on popup dialog
            trigger_view_menu_option(menu_paths[1], on_restore_default_layout_trigger)

            # Verify proper panes are open with Default Layout
            console_open = general.is_pane_visible('Console')
            asset_browser_open = general.is_pane_visible('Asset Browser')
            self.test_success = self.test_success and console_open and asset_browser_open
            general.idle_wait(2.0)

            # Trigger Load of saved custom layout
            trigger_view_menu_option(menu_paths[2], on_custom_layout_load_trigger)

            # Verify proper panes are open with saved custom layout
            landscape_canvas_open = general.is_pane_visible('Landscape Canvas')
            track_view_open = general.is_pane_visible('Track View')
            ui_editor_open = general.is_pane_visible('UI Editor')
            self.test_success = self.test_success and landscape_canvas_open and track_view_open and ui_editor_open
            general.idle_wait(2.0)

            # Restore to defaults and delete custom layout
            trigger_view_menu_option(menu_paths[1], on_restore_default_layout_trigger)
            trigger_view_menu_option(menu_paths[3], on_delete_custom_layout_trigger)

            # Verify that custom layout has been cleaned up
            pyside_utils.get_action_for_menu_path(editor_window, "View", *menu_paths[2])


test = TestLoadLayout()
test.run()
