"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.


C6312589: Tool Stability & Workflow: Asset Editor
https://testrail.agscollab.com/index.php?/cases/view/6312589
"""

import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import azlmbr.legacy.general as general
import Tests.ly_shared.pyside_utils as pyside_utils
import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.entity as EntityId
import azlmbr.math as math
import Tests.ly_shared.hydra_editor_utils as hydra
from PySide2 import QtWidgets
from PySide2.QtCore import Qt
from Tests.editor.editor_utils.editor_test_helper import EditorTestHelper


class ToolStabilityAssetEditorTest(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="tool_stability_asset_editor", args=["level"])

    @pyside_utils.wrap_async
    async def run_test(self):
        """
        Summary:
        Tool Stability & Workflow: Asset Editor

        Expected Behavior:
        1) The Asset Editor can be opened/closed.
        2) New Input Bindings can be created.
        3) Created input bindings populate the Pick Input Bindings list.
        4) Input Bindings can be opened.

        Test Steps:
        1) Open level
        2) Open Asset Editor
        3) Create new Input Bindings
        4) Open Save As dialog
        5) Close the Asset Editor
        6) Create new entity and add a Input component
        7) Assign a .inputbindings file to the input component
        8) Open Asset Editor through the newly assigned inputbinding file

        Note:
        - This test file must be called from the Lumberyard Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """

        def open_asset_editor():
            if general.is_pane_visible("Asset Editor"):
                return True
            general.open_pane("Asset Editor")
            return general.is_pane_visible("Asset Editor")

        def close_asset_editor():
            asset_editor.close()
            return not general.is_pane_visible("Asset Editor")

        class InputBinding:
            input_binding_name = None

        # 1) Open level
        self.test_success = self.create_level(
            self.args["level"],
            heightmap_resolution=1024,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=4096,
            use_terrain=True,
        )

        editor_window = pyside_utils.get_editor_main_window()

        # 2) Open Asset Editor
        print(f"Asset Editor opened: {open_asset_editor()}")

        # 3) Create new Input Bindings
        # Access Asset editor
        asset_editor = editor_window.findChild(QtWidgets.QDockWidget, "Asset Editor")
        asset_editor_widget = asset_editor.findChild(QtWidgets.QWidget, "m_assetEditorWidget")
        menu_bar = asset_editor_widget.findChild(QtWidgets.QMenuBar)
        # Get the action File->New->Input Bindings
        action = pyside_utils.find_child_by_pattern(menu_bar, "Input Bindings")
        # Trigger the action
        action.trigger()
        # wait until the trigger loads the frame
        await pyside_utils.wait_for_condition(lambda: len(asset_editor_widget.findChildren(QtWidgets.QFrame, "Input Event Groups")) == 1, 3.0)
        input_event_groups = asset_editor_widget.findChild(QtWidgets.QFrame, "Input Event Groups")
        # Click on + button to assign Input event groups
        input_event_groups.findChildren(QtWidgets.QToolButton)[1].click()
        # wait until the click action loads the frame
        await pyside_utils.wait_for_condition(lambda: len(asset_editor_widget.findChildren(QtWidgets.QFrame, "Event Name")) == 1, 3.0)
        new_event_frame = asset_editor_widget.findChild(QtWidgets.QFrame, "<Unspecified Event>")
        expander = new_event_frame.findChild(QtWidgets.QCheckBox, "")
        expander.click()
        event_name_frame = asset_editor_widget.findChild(QtWidgets.QFrame, "Event Name")
        event_name = event_name_frame.findChildren(QtWidgets.QLineEdit)[0]
        # Set some event name
        event_name.setText("tmp_input_binding")

        # 4) Open Save As dialog
        action = pyside_utils.find_child_by_pattern(menu_bar, {"iconText": "Save As"})
        # We use trigger_action_async here since it will open a modal dialog (save as)
        pyside_utils.trigger_action_async(action)
        # This checks if the Save As dialog is opened
        # NOTE: At the moment since we are not able to give a name and save something using QFileDialog
        # widget and for this particular case it is not so important to actually save the file, we are
        # just checking if the Save As dialog is opened and then closing it immediately
        active_modal_widget = await pyside_utils.wait_for_modal_widget()
        if active_modal_widget and "Save As" in active_modal_widget.windowTitle():
            print("Save as dialog opened")
            active_modal_widget.close()

        # 5) Close the Asset Editor
        print(f"Asset Editor closed: {close_asset_editor()}")

        # 6) Create new entity and add a Input component
        entity_position = math.Vector3(125.0, 136.0, 32.0)
        entity_id = editor.ToolsApplicationRequestBus(
            bus.Broadcast, "CreateNewEntityAtPosition", entity_position, EntityId.EntityId()
        )
        hydra.add_component("Input", entity_id)
        entity_name = editor.EditorEntityInfoRequestBus(bus.Event, "GetName", entity_id)

        # 7) Assign a .inputbindings file to the input component
        general.clear_selection()
        general.select_object(entity_name)
        general.open_pane("Entity Inspector")
        entity_inspector = editor_window.findChild(QtWidgets.QDockWidget, "Entity Inspector")
        await pyside_utils.wait_for_condition(lambda: len(entity_inspector.findChildren(QtWidgets.QPushButton, "attached-button")) == 1, 2.0)
        # Assess the file picker button of the "Input" component
        button = entity_inspector.findChild(QtWidgets.QPushButton, "attached-button")
        # Click on the file picker button and select a .inputbinding file
        # We use click_button_async here since it will open a modal dialog
        pyside_utils.click_button_async(button)
        active_modal_widget = await pyside_utils.wait_for_modal_widget()
        # This is to deal with the file picker for .inputbinding
        if active_modal_widget:
            tree = active_modal_widget.findChild(QtWidgets.QTreeView, "m_assetBrowserTreeViewWidget")
            # Make sure the folder structure tree is expanded so that after giving the searchtext the first
            # .inputbinding file is selected
            tree.expandAll()
            search_text = active_modal_widget.findChild(QtWidgets.QLineEdit, "textSearch")
            # Set the searchText so that the first .inputbinding file in the folder structure is selected
            search_text.setText(".inputbindings")
            # Save the file name for later to check when we open the Asset Editor
            InputBinding.input_binding_name = tree.currentIndex().data(Qt.DisplayRole)
            # Ensure the currently selected item actually is a .inputbinding file
            await pyside_utils.wait_for_condition(lambda: ".inputbinding" in str(tree.currentIndex().data(Qt.DisplayRole)))
            if ".inputbinding" in str(tree.currentIndex().data(Qt.DisplayRole)):
                print(".inputbinding file is selected in the file picker")
            # Click OK
            button_box = active_modal_widget.findChild(QtWidgets.QDialogButtonBox, "m_buttonBox")
            button_box.button(QtWidgets.QDialogButtonBox.Ok).click()

        # 8) Open Asset Editor through the newly assigned inputbinding file
        # Access the popup button to open the Asset Editor
        tool_button_parent = entity_inspector.findChild(QtWidgets.QFrame, "browse-edit").parent()
        tool_button = tool_button_parent.findChild(QtWidgets.QToolButton)
        initial_active_window = QtWidgets.QApplication.activeWindow()
        # Click on the "Open in Input Bindings Editor" button to open the selected .inputbindings file in the
        # Asset Editor
        tool_button.click()
        await pyside_utils.wait_for_condition(lambda: initial_active_window != QtWidgets.QApplication.activeWindow(), 3.0)
        current_active_window = QtWidgets.QApplication.activeWindow()
        # Verify if the newly opened window is actually the Asset Editor to edit the selected inputbinding
        if current_active_window.findChildren(QtWidgets.QDockWidget, InputBinding.input_binding_name):
            print("Asset Editor for the assigned inputbinding file is opened")


test = ToolStabilityAssetEditorTest()
test.run()
