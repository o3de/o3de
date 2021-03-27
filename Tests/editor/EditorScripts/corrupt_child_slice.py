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
C2603859: Parent Slice should still load on map if a child slice is corrupted.
https://testrail.agscollab.com/index.php?/cases/view/2603859
"""

import os
import sys
import shutil
import azlmbr.qt_helpers

import azlmbr
import azlmbr.legacy.general as general
import azlmbr.editor as editor
import azlmbr.bus as bus
import azlmbr.slice as slice
import azlmbr.asset as asset
import azlmbr.math as math

import Tests.ly_shared.pyside_utils as pyside_utils
from PySide2 import QtWidgets, QtTest, QtCore, QtGui
from Tests.editor.editor_utils.editor_test_helper import EditorTestHelper
from azlmbr.entity import EntityId

class TestCorruptChildSlice(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="corrupt_child_slice: ", args=["level"])

    @pyside_utils.wrap_async
    async def run_test(self):
        """
        Summary:
        Carry out various operations to ensure a corrupt child slice does not
        prevent a parent slice from loading.

        Expected Behavior:
        Parent layer of corrupt slice can be loaded.

        Test Steps:
        1) Create a new entity named "CorruptC".
        2) Save as a slice.
        3) Create a new entity named "CorruptB".
        4) Make CorruptC a child of CorruptB.
        5) Save CorruptB as a slice.
        6) Create a new entity named CorruptA.
        7) Make CorruptB a child of CorruptA.
        8) Save CorruptA as a slice.
        9) Instantiate an instance of CorruptB.
        10) Save the level and close the editor.
        11) Rename the CorruptC slice to CorruptC.slice.backup.
        12) Make a copy of CorruptB.slice named CorruptB.slice.backup.
        13) Open the editor and level.
        14) Check that an error is displayed when loading the level.
        15) Check that CorruptB and A have loaded but CorruptA is absent.
        16) Check that CorruptA has a blue icon.
        17) Check that CorruptB is orange.
        18) Close the error message.
        19) Use the context menu of CorruptB to enter the Advanced Save dialog.
        20) Check there is a warning message in the dialog.
        21) Check that either CorruptB has "changed" next to it or CorruptB is listed as
            having invalid references removed.
        22) Press "Save Selected Overrides".
        23) Close the editor and reopen the same level again.
        24) Check there are no warnings and the level loads correctly.
        25) Create a second instance of CorruptA.
        26) Check there are no errors.

        Note:
        - This test file must be called from the Lumberyard Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """

        Outliner_Name = "Entity Outliner (PREVIEW)"
        Console_Name = "Console"

        def fail_test(msg):
            print(msg)
            print("Test failed.")
            sys.exit()

        async def save_level_as(main_editor_window, level_name_text):
            save_action = pyside_utils.get_action_for_menu_path(main_editor_window, "File", "Save As")
            pyside_utils.trigger_action_async(save_action)
            active_widget = await pyside_utils.wait_for_modal_widget()

            save_level_as_dialogue = active_widget.findChild(QtWidgets.QWidget, "LevelFileDialog")
            level_name = await pyside_utils.wait_for_child_by_hierarchy(
                save_level_as_dialogue,
                ...,
                dict(type=QtWidgets.QLineEdit, objectName="nameLineEdit"),
            )
            level_name.setText(level_name_text)

            button_box = save_level_as_dialogue.findChild(QtWidgets.QDialogButtonBox, "buttonBox")
            button_box.button(QtWidgets.QDialogButtonBox.Ok).click()

            await pyside_utils.close_modal(active_widget)

        async def open_level_check_for_warning(level_name):
            general.open_level_no_prompt(level_name)

            try:
                dialog = await pyside_utils.wait_for_child_by_hierarchy(None,
                                                               {"objectName": "ErrorLogDialog", "type": QtWidgets.QDialog},
                                                                timeout=2.0)

                if dialog is None:
                    return False

                active_widget = QtWidgets.QApplication.activeModalWidget()

                button = active_widget.findChild(QtWidgets.QPushButton, "okButton")
                if button is not None:
                    button.click()
            except pyside_utils.EventLoopTimeoutException:
                # The assertion is a timeout while waiting for the dialog.
                return False

            return True

        async def push_slice_confirm_overwrite(button):
            # If a confirm dialog pops up, press the confirm button.
            pyside_utils.click_button_async(button)

            try:
                dialog = await pyside_utils.wait_for_child_by_hierarchy(None,
                                                                        {"objectName": "SliceUtilities.warningMessageBox",
                                                                         "type": QtWidgets.QMessageBox},
                                                                        timeout=1.0)
                confirm_button = pyside_utils.find_child_by_pattern(dialog, "Confirm")
                pyside_utils.click_button_async(confirm_button)
                await pyside_utils.close_modal(dialog)
            except pyside_utils.EventLoopTimeoutException:
                # No overwrite dialog appeared.
                return False

            return True

        async def push_slice_to_file(slice_entity_id):
            editor.ToolsApplicationRequestBus(bus.Broadcast, 'SetSelectedEntities', [slice_entity_id])
            general.idle_wait(0.0)
            pyside_utils.run_soon(lambda: slice.SliceRequestBus(bus.Broadcast, "ShowPushDialog", [slice_entity_id]))
            active_widget = await pyside_utils.wait_for_modal_widget()

            tree_widget = active_widget.findChild(QtWidgets.QTreeWidget, "SlicePushWidget.m_fieldTree")
            if tree_widget is None:
                return False

            # Check warning message
            warning_message = active_widget.findChild(QtWidgets.QLabel, "SlicePushWidget.m_warningTitle")
            if warning_message is None:
                fail_test("Failed to find warning message in push dialog.")

            if not warning_message.text().startswith("1 missing reference(s)"):
                fail_test("Failed to find Invalid References warning message.")

            # Check for CorruptB in the field selection.
            found_corrupt_b_message = False
            iterator = QtWidgets.QTreeWidgetItemIterator(tree_widget)
            while iterator.value():
                item = iterator.value()
                # Check it has one of the two possible messages.
                if item.text(0) == "CorruptB (invalid references will be removed)" or item.text(
                        0) == "CorruptB (changed)":
                    found_corrupt_b_message = True
                iterator += 1

            if not found_corrupt_b_message:
                fail_test("Failed to find CorruptB invalid reference removal message.")

            # Press the save button.
            save_button = pyside_utils.find_child_by_pattern(active_widget, "Save Selected Overrides")

            overwrite_result = await push_slice_confirm_overwrite(save_button)

            await pyside_utils.close_modal(active_widget)

            return overwrite_result

        def create_entity_with_name(entity_name):
            new_entity_id = editor.ToolsApplicationRequestBus(bus.Broadcast, 'CreateNewEntity', EntityId())
            if not new_entity_id.IsValid():
                fail_test("Failed to create new entity.")
            editor.EditorEntityAPIBus(bus.Event, 'SetName', new_entity_id, entity_name)
            name = editor.EditorEntityInfoRequestBus(bus.Event, 'GetName', new_entity_id)
            return new_entity_id

        def path_is_valid_asset(asset_path):
            tmp_asset_id = asset.AssetCatalogRequestBus(bus.Broadcast, "GetAssetIdByPath", asset_path, math.Uuid(), False)
            return tmp_asset_id.invoke("IsValid")

        def save_entity_as_slice(entity_id, slice_name):
            current_level_name = editor.EditorToolsApplicationRequestBus(bus.Broadcast, "GetCurrentLevelName")
            slice_file = slice_name + ".slice"
            slice_dir = os.path.join("Levels", current_level_name)
            full_path = os.path.join(slice_dir, slice_file)
            slice_created = slice.SliceRequestBus(bus.Broadcast, "CreateNewSlice", entity_id, full_path)
            self.wait_for_condition(lambda: path_is_valid_asset(full_path), 10.0)
            if not slice_created:
                fail_test("Failed to create slice.")
            general.idle_wait(0.0)

            return slice_dir, slice_file

        def check_log_for_text_absence(console_text_edit, text_to_check):
            # Grab the log text
            text = console_text_edit.toPlainText()

            # Scan it for failed save lines.
            lines = text.split('\n')

            for line in lines:
                if line.find(text_to_check) >= 0:
                    return False

            return True

        EditorTestHelper.after_level_load(self)

        app = QtWidgets.QApplication.instance()
        editor_window = pyside_utils.get_editor_main_window()
        main_window = editor_window.findChild(QtWidgets.QMainWindow)

        # Make sure the outliner and console are open.
        general.open_pane(Outliner_Name)
        entity_outliner = editor_window.findChild(QtWidgets.QDockWidget, Outliner_Name)
        if entity_outliner is None:
            fail_test("Failed to find the outliner.")

        outliner_main_window = entity_outliner.findChild(QtWidgets.QMainWindow)
        if outliner_main_window is None:
            fail_test("Failed to find the outliner main window.")

        outliner_tree_view = outliner_main_window.findChild(QtWidgets.QTreeView, "m_objectTree")
        if outliner_tree_view is None:
            fail_test("Failed to find the outliner tree view.")

        general.open_pane(Console_Name)
        console = main_window.findChild(QtWidgets.QDockWidget, Console_Name)
        if console is None:
            fail_test("Failed to find the console.")

        text_edit = console.findChild(QtWidgets.QPlainTextEdit, "textEdit")
        if text_edit is None:
            fail_test("Failed to find console textEdit.")

        # Create a level.
        result = general.create_level_no_prompt("test_level_1", 1024, 1, 4096, True)
        if result != 0:
            fail_test("Failed to create level")
        general.idle_wait(2.0)

        # Save as a different level name so we can switch between them.
        await save_level_as(editor_window, "test_level_2")

        # Create the entities.
        corrupt_a_id = create_entity_with_name("CorruptA")
        corrupt_b_id = create_entity_with_name("CorruptB")
        corrupt_c_id = create_entity_with_name("CorruptC")

        # Make C a slice.
        slice_file_path, slice_c_file = save_entity_as_slice(corrupt_c_id, "CorruptC")

        # Make C a child of B.
        azlmbr.components.TransformBus(bus.Event, "SetParent", corrupt_c_id, corrupt_b_id)

        # Make B a slice.
        slice_file_path, slice_b_file = save_entity_as_slice(corrupt_b_id, "CorruptB")

        # Make B a child of A.
        azlmbr.components.TransformBus(bus.Event, "SetParent", corrupt_b_id, corrupt_a_id)

        # Make A a slice.
        slice_file_path, slice_a_file = save_entity_as_slice(corrupt_a_id, "CorruptA")

        # Instantiate another B
        slice_path = os.path.join(slice_file_path, slice_b_file)
        asset_id = asset.AssetCatalogRequestBus(bus.Broadcast, "GetAssetIdByPath", slice_path, math.Uuid(), False)
        transform = math.Transform_CreateIdentity()
        slice.SliceRequestBus(bus.Broadcast, "InstantiateSliceFromAssetId", asset_id, transform)
        general.idle_wait(0.0)

        # Save.
        general.save_level()

        # Switch levels while the files are manipulated.
        general.open_level_no_prompt("test_level_1")
        if editor.EditorToolsApplicationRequestBus(bus.Broadcast, "GetCurrentLevelName") != "test_level_1":
            fail_test("Failed to reload test_level_1")

        # Rename the slice C file.
        game_folder = editor.EditorToolsApplicationRequestBus(bus.Broadcast, 'GetGameFolder')
        src = os.path.join(game_folder, slice_file_path, slice_c_file)
        dest = src + ".backup"
        os.rename(src, dest)

        # Duplicate the slice B file and rename that.
        src = os.path.join(game_folder, slice_file_path, slice_b_file)
        dest = src + ".backup"
        shutil.copyfile(src, dest)

        # Switch back to the level with the slice instances in and check for error messages.
        seen_error_dialog = await open_level_check_for_warning("test_level_2")

        if not seen_error_dialog:
            fail_test("Failed to find error message dialog.")

        if editor.EditorToolsApplicationRequestBus(bus.Broadcast, "GetCurrentLevelName") != "test_level_2":
            fail_test("Failed to reload test_level_2")

        # Check that A and B are present but not C
        corrupt_a_id = general.find_editor_entity("CorruptA")
        if not corrupt_a_id.isValid():
            fail_test("No CorruptA entity found after reload")
        corrupt_b_id = general.find_editor_entity("CorruptB")
        if not corrupt_b_id.isValid():
            fail_test("No CorruptB entity found after reload")
        corrupt_c_id = general.find_editor_entity("CorruptC")
        if corrupt_c_id.isValid():
            fail_test("CorruptC entity found after reload")

        # Pop up the advanced push dialog for CorruptB and push it.
        push_success = await push_slice_to_file(corrupt_b_id)
        if not push_success:
            fail_test("Failed to push slice.")

        # Switch levels again so we can reload level2.
        general.open_level_no_prompt("test_level_1")
        if editor.EditorToolsApplicationRequestBus(bus.Broadcast, "GetCurrentLevelName") != "test_level_1":
            fail_test("Failed to reload test_level_1")

        # Switch back to level 2 and check the error dialog did not appear.
        seen_error_dialog = await open_level_check_for_warning("test_level_2")

        if seen_error_dialog:
            fail_test("Level failed to load without errors.")

        # Instantiate another CorruptA and check there are no errors.
        # Any errors will appear in the console so we need to check that.
        text_edit.clear()

        slice_path = os.path.join(slice_file_path, slice_a_file)
        asset_id = asset.AssetCatalogRequestBus(bus.Broadcast, "GetAssetIdByPath", slice_path, math.Uuid(), False)
        transform = math.Transform_CreateIdentity()
        slice.SliceRequestBus(bus.Broadcast, "InstantiateSliceFromAssetId", asset_id, transform)
        general.idle_wait(0.0)

        if not check_log_for_text_absence(text_edit, "could not be loaded"):
            fail_test("Error occurred instantiating CorruptA.")

        print("Corrupt child slice test complete.")


test = TestCorruptChildSlice()
test.run()
