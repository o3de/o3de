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
C1506899: Unsaved Changes Pop-Up
https://testrail.agscollab.com/index.php?/cases/view/1506899
"""

import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import azlmbr.legacy.general as general
import azlmbr.asset as asset
import azlmbr.editor as editor
import azlmbr.math as math
import azlmbr.bus as bus
import Tests.ly_shared.pyside_utils as pyside_utils
from PySide2 import QtWidgets
from Tests.editor.editor_utils.editor_test_helper import EditorTestHelper


class UnsavedChangesPopupTest(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="unsaved_changes_popup", args=["level"])

    @pyside_utils.wrap_async
    async def run_test(self):
        """
        Summary:
        Verify if the unsaved change popup appears and each of the buttons function appropriately.

        Expected Behavior:
        A prompt appears informing the user there are unsaved changes, with 3 options present: Yes, No, and Cancel.
        All options function as expected.

        Test Steps:
        1) Open a new level
        2) Access Editor window
        3) Open .inputbindings file in Asset Editor
        4) Delete All events initially
        5) Add an input event to make a change
        6) Click on close - verify CANCEL function
        7) Click on close - verify YES function
        8) Make a new change for verifying NO button (Add another input event group)
        9) Click on close - verify NO function
        10) Close Asset Editor

        Note:
        - This test file must be called from the Lumberyard Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """

        INPUTBINDING_FILE_NAME = "test.inputbindings"

        def close_asset_editor():
            general.close_pane("Asset Editor")
            general.close_pane(INPUTBINDING_FILE_NAME)
            return not general.is_pane_visible("Asset Editor")

        async def close_asset_editor_unsaved(button_type):
            asset_editor = editor_window.findChild(QtWidgets.QDockWidget, INPUTBINDING_FILE_NAME)

            # Trigger the closing of the Asset Editor as async so we can listen for the modal dialog
            pyside_utils.run_soon(lambda: asset_editor.close())
            active_modal_widget = await pyside_utils.wait_for_modal_widget(timeout=2.0)
            if active_modal_widget:
                print("Message Box opened")
                message_box = active_modal_widget.findChild(QtWidgets.QMessageBox, "")
                button = message_box.button(button_type)
                pyside_utils.click_button_async(button)

            return await pyside_utils.wait_for_destroyed(active_modal_widget)

        # 1) Open a new level
        self.test_success = self.create_level(
            self.args["level"],
            heightmap_resolution=1024,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=4096,
            use_terrain=True,
        )

        # 2) Access Editor window
        editor_window = pyside_utils.get_editor_main_window()

        # 3) Open .inputbindings file in Asset Editor
        # Close Asset Editor initially
        close_asset_editor()
        input_bindings_id = asset.AssetCatalogRequestBus(
            bus.Broadcast, "GetAssetIdByPath", INPUTBINDING_FILE_NAME, math.Uuid(), False
        )
        editor.AssetEditorRequestsBus(bus.Broadcast, "OpenAssetEditorById", input_bindings_id)

        asset_editor = editor_window.findChild(QtWidgets.QDockWidget, INPUTBINDING_FILE_NAME)
        asset_editor_widget = asset_editor.findChild(QtWidgets.QWidget, "m_assetEditorWidget")
        input_event_groups = await pyside_utils.wait_for(lambda: asset_editor_widget.findChild(QtWidgets.QFrame, "Input Event Groups"))

        # 4) Delete All events initially
        delete_all_button = input_event_groups.findChildren(QtWidgets.QToolButton, "")[1]
        pyside_utils.click_button_async(delete_all_button)
        active_modal_widget = await pyside_utils.wait_for_modal_widget()
        if active_modal_widget:
            message_box = active_modal_widget.findChild(QtWidgets.QMessageBox, "")
            button = message_box.button(QtWidgets.QMessageBox.Yes)
            button.click()

        # 5) Add an input event to make a change
        await pyside_utils.wait_for_condition(lambda: len(asset_editor_widget.findChildren(QtWidgets.QFrame, "Input Event Groups")) > 1, 2.0)
        input_event_groups = asset_editor_widget.findChildren(QtWidgets.QFrame, "Input Event Groups")[1]
        add_button = input_event_groups.findChild(QtWidgets.QToolButton, "")
        add_button.click()

        # 6) Click on close - verify CANCEL function
        await close_asset_editor_unsaved(QtWidgets.QMessageBox.Cancel)
        # Cancel should not change anything and the Asset Editor should remain open
        print(f"'CANCEL' button working as expected: {general.is_pane_visible(INPUTBINDING_FILE_NAME)}")

        # 7) Click on close - verify YES function
        await close_asset_editor_unsaved(QtWidgets.QMessageBox.Yes)
        # Yes option should save the changes and not close the Asset Editor
        print(f"Asset Editor is visible after clicking YES: {general.is_pane_visible(INPUTBINDING_FILE_NAME)}")
        status_bar = await pyside_utils.wait_for(lambda: asset_editor_widget.findChild(QtWidgets.QWidget, "AssetEditorStatusBar"))
        text_edit = status_bar.findChild(QtWidgets.QLabel, "textEdit")
        success = await pyside_utils.wait_for_condition(lambda: f"{INPUTBINDING_FILE_NAME} - Asset saved!" in text_edit.text(), 3.0)
        if success:
            print("'YES' button working as expected")

        # 8) Make a new change for verifying NO button (Add another input event group)
        input_event_groups = await pyside_utils.wait_for(lambda: asset_editor_widget.findChild(QtWidgets.QFrame, "Input Event Groups"), 2.0)
        add_button = await pyside_utils.wait_for(lambda: input_event_groups.findChild(QtWidgets.QToolButton, ""), 2.0)
        add_button.click()

        # 9) Click on close - verify NO function
        await close_asset_editor_unsaved(QtWidgets.QMessageBox.No)
        # No button should close the Asset Editor and changes should not be saved
        print(f"Asset Editor is not visible after clicking NO: {not general.is_pane_visible('Asset Editor')}")

        # Reopen inputbindings file
        input_bindings_id = asset.AssetCatalogRequestBus(
            bus.Broadcast, "GetAssetIdByPath", INPUTBINDING_FILE_NAME, math.Uuid(), False
        )
        editor.AssetEditorRequestsBus(bus.Broadcast, "OpenAssetEditorById", input_bindings_id)
        # Ensure there are no changes saved (only one event group should be there)
        asset_editor = await pyside_utils.wait_for(lambda: editor_window.findChild(QtWidgets.QDockWidget, INPUTBINDING_FILE_NAME), 2.0)
        asset_editor_widget = asset_editor.findChild(QtWidgets.QWidget, "m_assetEditorWidget")
        input_event_groups = asset_editor_widget.findChild(QtWidgets.QFrame, "Input Event Groups")
        no_of_elements_label = input_event_groups.findChild(QtWidgets.QLabel, "DefaultLabel")
        success = await pyside_utils.wait_for_condition(lambda: "1 elements" in no_of_elements_label.text(), 2.0)
        if success:
            print("'NO' button working as expected")

        # 10) Close Asset Editor
        print(f"Asset Editor closed: {close_asset_editor()}")


test = UnsavedChangesPopupTest()
test.run()
