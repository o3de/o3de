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
C1506875: Open Input Bindings File
https://testrail.agscollab.com/index.php?/cases/view/1506875

C1506876: Save Input Bindings File
https://testrail.agscollab.com/index.php?/cases/view/1506876
"""

import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import azlmbr.legacy.general as general
import Tests.ly_shared.pyside_utils as pyside_utils
from PySide2 import QtWidgets, QtTest
from PySide2.QtCore import Qt
from Tests.editor.editor_utils.editor_test_helper import EditorTestHelper


class OpenSaveInputBindingFileTest(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="open_save_input_binding_file", args=["level"])

    @pyside_utils.wrap_async
    async def run_test(self):
        """
        Summary:
        Verify if we are able to open/save a .inputbindings file in Asset Editor

        Expected Behavior:
        The .inputbindings asset is loaded into the Asset Editor, replacing any currently open files.
        The .inputbindings file is saved correctly.
        All changes from step 1. are present in the .inputbindings file.

        Test Steps:
        1) Open a new level
        2) Open Asset Editor
        3) Access Asset Editor
        4) Open .inputbindings file
        5) Verify if the file has loaded by verifying the status bar in Asset Editor
        6) Delete all input events initially
        7) Add an event and save the file
        8) Close Asset editor and reopen the inputbindings file
        9) Verify if changes are saved.
        10) Close Asset Editor

        Note:
        - This test file must be called from the Lumberyard Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """

        INPUTBINDING_FILE_NAME = "test.inputbindings"

        def open_asset_editor():
            general.open_pane("Asset Editor")
            return general.is_pane_visible("Asset Editor")

        def close_asset_editor():
            general.close_pane("Asset Editor")
            return not general.is_pane_visible("Asset Editor")

        async def open_input_binding_file():
            # Trigger the open asynchronously, since it opens a modal dialog
            action = pyside_utils.find_child_by_pattern(asset_editor_widget, {"iconText": "Open"})
            pyside_utils.trigger_action_async(action)

            # This is to deal with the file picker for .inputbinding
            active_modal_widget = await pyside_utils.wait_for_modal_widget()
            tree = active_modal_widget.findChild(QtWidgets.QTreeView, "m_assetBrowserTreeViewWidget")
            # Make sure the folder structure tree is expanded so that after giving the searchtext the first
            # .inputbinding file is selected
            tree.expandAll()
            search_text = active_modal_widget.findChild(QtWidgets.QLineEdit, "textSearch")
            # Set the searchText so that the first .inputbinding file in the folder structure is selected
            search_text.setText(INPUTBINDING_FILE_NAME)
            model_index = pyside_utils.find_child_by_pattern(tree, INPUTBINDING_FILE_NAME)
            pyside_utils.item_view_index_mouse_click(tree, model_index)
            # Click OK
            button_box = active_modal_widget.findChild(QtWidgets.QDialogButtonBox, "m_buttonBox")
            button_box.button(QtWidgets.QDialogButtonBox.Ok).click() 

        # 1) Open a new level
        self.test_success = self.create_level(
            self.args["level"],
            heightmap_resolution=1024,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=4096,
            use_terrain=True,
        )

        # 2) Open Asset Editor
        print(f"Asset Editor opened: {open_asset_editor()}")

        # 3) Access Asset Editor
        editor_window = pyside_utils.get_editor_main_window()
        asset_editor = editor_window.findChild(QtWidgets.QDockWidget, "Asset Editor")
        asset_editor_widget = asset_editor.findChild(QtWidgets.QWidget, "m_assetEditorWidget")

        # 4) Open .inputbindings file
        await open_input_binding_file()

        # 5) Verify if the file has loaded by verifying the status bar in Asset Editor
        status_bar = asset_editor_widget.findChild(QtWidgets.QWidget, "AssetEditorStatusBar")
        text_edit = status_bar.findChild(QtWidgets.QLabel, "textEdit")
        success = await pyside_utils.wait_for_condition(lambda: f"{INPUTBINDING_FILE_NAME} - Asset loaded!" in text_edit.text(), 2.0)
        if success:
            print("Input Binding File opened in the Asset Editor")

        # C1506876
        # 6) Delete all input events initially
        await pyside_utils.wait_for_condition(lambda: asset_editor_widget.findChild(QtWidgets.QFrame, "Input Event Groups") is not None)
        input_event_groups = asset_editor_widget.findChild(QtWidgets.QFrame, "Input Event Groups")
        delete_all_button = input_event_groups.findChildren(QtWidgets.QToolButton, "")[1]
        pyside_utils.click_button_async(delete_all_button)

        # Clicking the Delete All button will prompt the user if they are sure they want to
        # delete all the entries, so we wait for this modal dialog and then accept it
        active_modal_widget = await pyside_utils.wait_for_modal_widget()
        message_box = active_modal_widget.findChild(QtWidgets.QMessageBox)
        yes_button = message_box.button(QtWidgets.QMessageBox.Yes)
        yes_button.click()

        # 7) Add an event and save the file
        # First QToolButton is +, Second QToolButton is Delete
        await pyside_utils.wait_for_condition(lambda: len(asset_editor_widget.findChildren(QtWidgets.QFrame, "Input Event Groups")) > 1, 2.0)
        input_event_groups = asset_editor_widget.findChildren(QtWidgets.QFrame, "Input Event Groups")[1]
        add_button = input_event_groups.findChild(QtWidgets.QToolButton, "")
        add_button.click()
        action = pyside_utils.find_child_by_pattern(asset_editor_widget, {"text": "&Save"})
        action.trigger()
        await pyside_utils.wait_for_condition(lambda: "Asset saved!" in text_edit.text(), 2.0)

        # 8) Close Asset editor and reopen the inputbindings file
        close_asset_editor()
        open_asset_editor()
        await open_input_binding_file()

        # 9) Verify if changes are saved. (1 Event should be present)
        # We need to re-find the Asset Editor and all its child widgets since we
        # close/re-opened it, so it is brand new
        asset_editor = editor_window.findChild(QtWidgets.QDockWidget, "Asset Editor")
        asset_editor_widget = asset_editor.findChild(QtWidgets.QWidget, "m_assetEditorWidget")
        input_event_groups = asset_editor_widget.findChild(QtWidgets.QFrame, "Input Event Groups")
        no_of_elements_label = input_event_groups.findChild(QtWidgets.QLabel, "DefaultLabel")
        success = await pyside_utils.wait_for_condition(lambda: "1 elements" in no_of_elements_label.text(), 3.0)
        if success:
            print("Changes are saved successfully")

        # 6) Close Asset Editor
        print(f"Asset Editor closed: {close_asset_editor()}")


test = OpenSaveInputBindingFileTest()
test.run()
