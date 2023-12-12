"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


def AssetPicker_UI_UX():

    import pyside_utils

    @pyside_utils.wrap_async
    async def run_test():
        """
        Summary:
        Verify the functionality of Asset Picker and UI/UX properties

        Expected Behavior:
        The asset picker opens and is labeled appropriately ("Pick Model Asset" in this instance).
        The Asset Picker window can be resized and moved around the screen.
        The file tree expands/retracts appropriately and a scroll bar is present when the menus extend 
        beyond the length of the window.
        The assets are limited to a valid type for the field selected (model assets in this instance)
        The asset picker is closed and the selected asset is assigned to the mesh component.

        Test Steps:
        1) Open a simple level
        2) Create entity and add Mesh component
        3) Access Entity Inspector
        4) Click Asset Picker (Model Asset)
            a) Collapse all the files initially and verify if scroll bar is not visible
            b) Expand/Verify Top folder of file path
            c) Expand/Verify Nested folder of file path
            d) Verify if the ScrollBar appears after expanding folders
            e) Collapse Nested and Top Level folders and verify if collapsed
            f) Verify if the correct files are appearing in the Asset Picker
            g) Move the widget and verify position
            h) Resize the widget
            g) Assign Model asset
        5) Verify if Model Asset is assigned via both OK/Enter options

        Note:
        - This test file must be called from the O3DE Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """

        import os
        from PySide2 import QtWidgets, QtTest, QtCore
        from PySide2.QtCore import Qt

        import azlmbr.asset as asset
        import azlmbr.bus as bus
        import azlmbr.editor as editor
        import azlmbr.entity as entity
        import azlmbr.legacy.general as general
        import azlmbr.math as math

        import editor_python_test_tools.hydra_editor_utils as hydra
        from editor_python_test_tools.utils import Report
        from editor_python_test_tools.utils import TestHelper as helper

        file_path = ["AutomatedTesting", "Assets", "Objects", "Foliage"]

        def select_entity_by_name(entity_name):
            searchFilter = entity.SearchFilter()
            searchFilter.names = [entity_name]
            entities = entity.SearchBus(bus.Broadcast, 'SearchEntities', searchFilter)
            editor.ToolsApplicationRequestBus(bus.Broadcast, 'MarkEntitySelected', entities[0])
            
        def is_asset_assigned(component, interaction_option):
            path = os.path.join("assets", "objects", "foliage", "cedar.fbx.azmodel")
            expected_asset_id = asset.AssetCatalogRequestBus(bus.Broadcast, 'GetAssetIdByPath', path, math.Uuid(),
                                                             False)
            result = hydra.get_component_property_value(component, "Controller|Configuration|Model Asset")
            expected_asset_str = expected_asset_id.invoke("ToString")
            result_str = result.invoke("ToString")
            Report.info(f"Asset assigned for {interaction_option} option: {expected_asset_str == result_str}")
            return expected_asset_str == result_str

        def move_and_resize_widget(widget):
            # Move the widget and verify position
            initial_position = widget.pos()
            x, y = initial_position.x() + 5, initial_position.y() + 5
            widget.move(x, y)
            curr_position = widget.pos()
            asset_picker_moved = (
                "Asset Picker widget moved successfully",
                "Failed to move Asset Picker widget"
            )
            Report.result(asset_picker_moved, curr_position.x() == x and curr_position.y() == y)

            # Resize the widget and verify size
            width, height = (
                widget.geometry().width() + 10,
                widget.geometry().height() + 10,
            )
            widget.resize(width, height)
            asset_picker_resized = (
                "Resized Asset Picker widget successfully",
                "Failed to resize Asset Picker widget"
            )
            Report.result(asset_picker_resized, widget.geometry().width() == width and widget.geometry().height() ==
                          height)

        def verify_expand(model_index, tree):
            initially_collapsed = (
                "Folder initially collapsed",
                "Folder unexpectedly expanded"
            )
            expanded = (
                "Folder expanded successfully",
                "Failed to expand folder"
            )
            # Check initial collapse
            Report.result(initially_collapsed, not tree.isExpanded(model_index))
            # Expand at the specified index
            tree.expand(model_index)
            # Verify expansion
            Report.result(expanded, tree.isExpanded(model_index))

        def verify_collapse(model_index, tree):
            collapsed = (
                "Folder hierarchy collapsed successfully",
                "Failed to collapse folder hierarchy"
            )
            tree.collapse(model_index)
            Report.result(collapsed, not tree.isExpanded(model_index))

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
                        Report.info(f"Incorrect file found: {cur_data}")
                        return False
                    indices.append(cur_index)
            return True

        async def asset_picker(allowed_asset_extensions, asset, interaction_option):
            active_modal_widget = await pyside_utils.wait_for_modal_widget()
            if active_modal_widget:
                dialog = active_modal_widget.findChildren(QtWidgets.QDialog, "AssetPickerDialogClass")[0]
                asset_picker_title = (
                    "Asset Picker window is titled as expected",
                    "Asset Picker window has an unexpected title"
                )
                Report.result(asset_picker_title, dialog.windowTitle() == "Pick ModelAsset")
                tree = dialog.findChildren(QtWidgets.QTreeView, "m_assetBrowserTreeViewWidget")[0]
                scroll_area = tree.findChild(QtWidgets.QWidget, "qt_scrollarea_vcontainer")
                scroll_bar = scroll_area.findChild(QtWidgets.QScrollBar)

                # a) Collapse all the files initially and verify if scroll bar is not visible
                tree.collapseAll()
                await pyside_utils.wait_for_condition(lambda: not scroll_bar.isVisible(), 0.5)
                scroll_bar_hidden = (
                    "Scroll Bar is not visible before tree expansion",
                    "Scroll Bar is visible before tree expansion"
                )
                Report.result(scroll_bar_hidden, not scroll_bar.isVisible())

                # Get Model Index of the file paths
                model_index_1 = pyside_utils.find_child_by_pattern(tree, file_path[0])
                model_index_2 = pyside_utils.find_child_by_pattern(model_index_1, file_path[1])

                # b) Expand/Verify Top folder of file path
                verify_expand(model_index_1, tree)

                # c) Expand/Verify Nested folder of file path
                verify_expand(model_index_2, tree)

                # d) Verify if the ScrollBar appears after expanding folders
                tree.expandAll()
                await pyside_utils.wait_for_condition(lambda: scroll_bar.isVisible(), 0.5)
                scroll_bar_visible = (
                    "Scroll Bar is visible after tree expansion",
                    "Scroll Bar is not visible after tree expansion"
                )
                Report.result(scroll_bar_visible, scroll_bar.isVisible())

                # e) Collapse Nested and Top Level folders and verify if collapsed
                verify_collapse(model_index_2, tree)
                verify_collapse(model_index_1, tree)

                # f) Verify if the correct files are appearing in the Asset Picker
                asset_picker_correct_files_appear = (
                    "Expected assets populated in the file picker",
                    "Found unexpected assets in the file picker"
                )
                Report.result(asset_picker_correct_files_appear, verify_files_appeared(tree.model(),
                                                                                       allowed_asset_extensions))

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

        # 1) Open an existing simple level
        hydra.open_base_level()

        # 2) Create entity and add Mesh component
        entity_position = math.Vector3(125.0, 136.0, 32.0)
        entity = hydra.Entity("TestEntity")
        entity.create_entity(entity_position, ["Mesh"])

        # 3) Access Entity Inspector
        editor_window = pyside_utils.get_editor_main_window()
        entity_inspector = editor_window.findChild(QtWidgets.QDockWidget, "Inspector")
        component_list_widget = entity_inspector.findChild(QtWidgets.QWidget, "m_componentListContents")

        # 4) Click on Asset Picker (Model Asset)
        select_entity_by_name("TestEntity")
        general.idle_wait(0.5)
        attached_button = component_list_widget.findChildren(QtWidgets.QPushButton, "attached-button")[0]

        # Assign Model Asset via OK button
        pyside_utils.click_button_async(attached_button)
        await asset_picker(["azmodel", "fbx"], "cedar.fbx (ModelAsset)", "ok")

        # 5) Verify if Model Asset is assigned
        try:
            mesh_success = await pyside_utils.wait_for_condition(lambda: is_asset_assigned(entity.components[0],
                                                                                           "ok"))
        except pyside_utils.EventLoopTimeoutException as err:
            print(err)
            mesh_success = False
        model_asset_assigned_ok = (
            "Successfully assigned Model asset via OK button",
            "Failed to assign Model asset via OK button"
        )
        Report.result(model_asset_assigned_ok, mesh_success)

        # Clear Model Asset
        hydra.get_set_test(entity, 0, "Controller|Configuration|Model Asset", None)
        select_entity_by_name("TestEntity")
        general.idle_wait(0.5)
        attached_button = component_list_widget.findChildren(QtWidgets.QPushButton, "attached-button")[0]

        # Assign Model Asset via Enter
        pyside_utils.click_button_async(attached_button)
        await asset_picker(["azmodel", "fbx"], "cedar.fbx (ModelAsset)", "enter")

        # 5) Verify if Model Asset is assigned
        try:
            mesh_success = await pyside_utils.wait_for_condition(lambda: is_asset_assigned(entity.components[0],
                                                                                           "enter"))
        except pyside_utils.EventLoopTimeoutException as err:
            print(err)
            mesh_success = False
        model_asset_assigned_enter = (
            "Successfully assigned Model Asset via Enter button",
            "Failed to assign Model Asset via Enter button"
        )
        Report.result(model_asset_assigned_enter, mesh_success)

    run_test()


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(AssetPicker_UI_UX)
