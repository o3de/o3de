"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

"""
C13751579: Asset Picker UI/UX
"""

import os
import sys
from PySide2 import QtWidgets, QtTest, QtCore
from PySide2.QtCore import Qt

import azlmbr.asset as asset
import azlmbr.bus as bus
import azlmbr.legacy.general as general
import azlmbr.paths
import azlmbr.math as math

sys.path.append(os.path.join(azlmbr.paths.devroot, 'AutomatedTesting', 'Gem', 'PythonTests'))
import editor_python_test_tools.hydra_editor_utils as hydra
import editor_python_test_tools.pyside_utils as pyside_utils
from editor_python_test_tools.editor_test_helper import EditorTestHelper


class AssetPickerUIUXTest(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="AssetPicker_UI_UX", args=["level"])

    @pyside_utils.wrap_async
    async def run_test(self):
        """
        Summary:
        Verify the functionality of Asset Picker and UI/UX properties

        Expected Behavior:
        The asset picker opens and is labeled appropriately ("Pick Model Asset" in this instance).
        The Asset Picker window can be resized and moved around the screen.
        The file tree expands/retracts appropriately and a scroll bar is present when the menus extend 
        beyond the length of the window.
        The assets are limited to a valid type for the field selected (mesh assets in this instance)
        The asset picker is closed and the selected asset is assigned to the mesh component.

        Test Steps:
        1) Open a new level
        2) Create entity and add Mesh component
        3) Access Entity Inspector
        4) Click Asset Picker (Mesh Asset)
            a) Collapse all the files initially and verify if scroll bar is not visible
            b) Expand/Verify Top folder of file path
            c) Expand/Verify Nested folder of file path
            d) Verify if the ScrollBar appears after expanding folders
            e) Collapse Nested and Top Level folders and verify if collapsed
            f) Verify if the correct files are appearing in the Asset Picker
            g) Move the widget and verify position
            h) Resize the widget
            g) Assign Mesh asset
        5) Verify if Mesh Asset is assigned via both OK/Enter options

        Note:
        - This test file must be called from the Lumberyard Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """

        self.file_path = ["AutomatedTesting", "Assets", "Objects", "Foliage"]
        self.incorrect_file_found = False
        self.mesh_asset = "cedar.azmodel"
        self.prefix = ""

        def is_asset_assigned(component, interaction_option):
            path = os.path.join("assets", "objects", "foliage", "cedar.azmodel")
            expected_asset_id = asset.AssetCatalogRequestBus(bus.Broadcast, 'GetAssetIdByPath', path, math.Uuid(),
                                                             False)
            result = hydra.get_component_property_value(component, "Controller|Configuration|Mesh Asset")
            expected_asset_str = expected_asset_id.invoke("ToString")
            result_str = result.invoke("ToString")
            print(f"Asset assigned for {interaction_option} option: {expected_asset_str == result_str}")
            return expected_asset_str == result_str

        def move_and_resize_widget(widget):
            # Move the widget and verify position
            initial_position = widget.pos()
            x, y = initial_position.x() + 5, initial_position.y() + 5
            widget.move(x, y)
            curr_position = widget.pos()
            move_success = curr_position.x() == x and curr_position.y() == y
            self.test_success = move_success and self.test_success
            self.log(f"Widget Move Test: {move_success}")

            # Resize the widget and verify size
            width, height = (
                widget.geometry().width() + 10,
                widget.geometry().height() + 10,
            )
            widget.resize(width, height)
            resize_success = widget.geometry().width() == width and widget.geometry().height() == height
            self.test_success = resize_success and self.test_success
            self.log(f"Widget Resize Test: {resize_success}")

        def verify_files_appeared(model, allowed_asset_extensions, parent_index=QtCore.QModelIndex()):
            indices = [parent_index]
            while len(indices) > 0:
                parent_index = indices.pop(0)
                for row in range(model.rowCount(parent_index)):
                    cur_index = model.index(row, 0, parent_index)
                    cur_data = cur_index.data(Qt.DisplayRole)
                    if (
                        "." in cur_data
                        and (cur_data.lower().split(".")[-1] not in allowed_asset_extensions)
                        and not cur_data[-1] == ")"
                    ):
                        print(f"Incorrect file found: {cur_data}")
                        self.incorrect_file_found = True
                        indices = list()
                        break
                    indices.append(cur_index)
            self.test_success = not self.incorrect_file_found and self.test_success

        def print_message_prefix(message):
            print(f"{self.prefix}: {message}")

        async def asset_picker(prefix, allowed_asset_extensions, asset, interaction_option):
            active_modal_widget = await pyside_utils.wait_for_modal_widget()
            if active_modal_widget and self.prefix == "":
                self.prefix = prefix
                dialog = active_modal_widget.findChildren(QtWidgets.QDialog, "AssetPickerDialogClass")[0]
                print_message_prefix(f"Asset Picker title for Mesh: {dialog.windowTitle()}")
                tree = dialog.findChildren(QtWidgets.QTreeView, "m_assetBrowserTreeViewWidget")[0]
                scroll_area = tree.findChild(QtWidgets.QWidget, "qt_scrollarea_vcontainer")
                scroll_bar = scroll_area.findChild(QtWidgets.QScrollBar)

                # a) Collapse all the files initially and verify if scroll bar is not visible
                tree.collapseAll()
                await pyside_utils.wait_for_condition(lambda: not scroll_bar.isVisible(), 0.5)
                print_message_prefix(
                    f"Scroll Bar is not visible before expanding the tree: {not scroll_bar.isVisible()}"
                )

                # Get Model Index of the file paths
                model_index_1 = pyside_utils.find_child_by_pattern(tree, self.file_path[0])
                print(model_index_1.model())
                model_index_2 = pyside_utils.find_child_by_pattern(model_index_1, self.file_path[1])

                # b) Expand/Verify Top folder of file path
                print_message_prefix(f"Top level folder initially collapsed: {not tree.isExpanded(model_index_1)}")
                tree.expand(model_index_1)
                print_message_prefix(f"Top level folder expanded: {tree.isExpanded(model_index_1)}")

                # c) Expand/Verify Nested folder of file path
                print_message_prefix(f"Nested folder initially collapsed: {not tree.isExpanded(model_index_2)}")
                tree.expand(model_index_2)
                print_message_prefix(f"Nested folder expanded: {tree.isExpanded(model_index_2)}")

                # d) Verify if the ScrollBar appears after expanding folders
                tree.expandAll()
                await pyside_utils.wait_for_condition(lambda: scroll_bar.isVisible(), 0.5)
                print_message_prefix(f"Scroll Bar appeared after expanding tree: {scroll_bar.isVisible()}")

                # e) Collapse Nested and Top Level folders and verify if collapsed
                tree.collapse(model_index_2)
                print_message_prefix(f"Nested folder collapsed: {not tree.isExpanded(model_index_2)}")
                tree.collapse(model_index_1)
                print_message_prefix(f"Top level folder collapsed: {not tree.isExpanded(model_index_1)}")

                # f) Verify if the correct files are appearing in the Asset Picker
                verify_files_appeared(tree.model(), allowed_asset_extensions)
                print_message_prefix(f"Expected Assets populated in the file picker: {not self.incorrect_file_found}")

                # While we are here we can also check if we can resize and move the widget
                move_and_resize_widget(active_modal_widget)

                # g) Assign asset
                tree.collapseAll()
                await pyside_utils.wait_for_condition(
                    lambda: len(dialog.findChildren(QtWidgets.QFrame, "m_searchWidget")) > 0, 0.5)
                search_widget = dialog.findChildren(QtWidgets.QFrame, "m_searchWidget")[0]
                search_line_edit = search_widget.findChildren(QtWidgets.QLineEdit, "textSearch")[0]
                search_line_edit.setText(asset)
                tree.expandAll()
                asset_model_index = pyside_utils.find_child_by_pattern(tree, asset)
                await pyside_utils.wait_for_condition(lambda: asset_model_index.isValid(), 2.0)
                tree.expand(asset_model_index)
                tree.setCurrentIndex(asset_model_index)
                if interaction_option == "ok":
                    button_box = dialog.findChild(QtWidgets.QDialogButtonBox, "m_buttonBox")
                    ok_button = button_box.button(QtWidgets.QDialogButtonBox.Ok)
                    await pyside_utils.click_button_async(ok_button)
                elif interaction_option == "enter":
                    QtTest.QTest.keyClick(tree, Qt.Key_Enter, Qt.NoModifier)
                self.prefix = ""

        # 1) Open a new level
        self.test_success = self.create_level(
            self.args["level"],
            heightmap_resolution=1024,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=4096,
            use_terrain=False,
        )

        # 2) Create entity and add Mesh component
        entity_position = math.Vector3(125.0, 136.0, 32.0)
        entity = hydra.Entity("TestEntity")
        entity.create_entity(entity_position, ["Mesh"])

        # 3) Access Entity Inspector
        editor_window = pyside_utils.get_editor_main_window()
        entity_inspector = editor_window.findChild(QtWidgets.QDockWidget, "Entity Inspector")
        component_list_widget = entity_inspector.findChild(QtWidgets.QWidget, "m_componentListContents")

        # 4) Click on Asset Picker (Mesh Asset)
        general.select_object("TestEntity")
        general.idle_wait(0.5)
        mesh_asset = component_list_widget.findChildren(QtWidgets.QFrame, "Mesh Asset")[0]
        attached_button = mesh_asset.findChildren(QtWidgets.QPushButton, "attached-button")[0]

        # Assign Mesh Asset via OK button
        pyside_utils.click_button_async(attached_button)
        await asset_picker("Mesh Asset", ["azmodel", "fbx"], "cedar (ModelAsset)", "ok")

        # 5) Verify if Mesh Asset is assigned
        try:
            mesh_success = await pyside_utils.wait_for_condition(lambda: is_asset_assigned(entity.components[0],
                                                                                           "ok"))
        except pyside_utils.EventLoopTimeoutException as err:
            print(err)
            mesh_success = False
        self.test_success = mesh_success and self.test_success

        # Clear Mesh Asset
        hydra.get_set_test(entity, 0, "Controller|Configuration|Mesh Asset", None)
        general.select_object("TestEntity")
        general.idle_wait(0.5)
        mesh_asset = component_list_widget.findChildren(QtWidgets.QFrame, "Mesh Asset")[0]
        attached_button = mesh_asset.findChildren(QtWidgets.QPushButton, "attached-button")[0]

        # Assign Mesh Asset via Enter
        pyside_utils.click_button_async(attached_button)
        await asset_picker("Mesh Asset", ["azmodel", "fbx"], "cedar (ModelAsset)", "enter")

        # 5) Verify if Mesh Asset is assigned
        try:
            mesh_success = await pyside_utils.wait_for_condition(lambda: is_asset_assigned(entity.components[0],
                                                                                           "enter"))
        except pyside_utils.EventLoopTimeoutException as err:
            print(err)
            mesh_success = False
        self.test_success = mesh_success and self.test_success


test = AssetPickerUIUXTest()
test.run()
