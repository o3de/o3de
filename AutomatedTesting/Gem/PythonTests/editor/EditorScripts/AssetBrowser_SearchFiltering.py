"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    asset_filtered = (
        "Asset was filtered to in the Asset Browser",
        "Failed to filter to the expected asset"
    )
    asset_type_filtered = (
        "Expected asset type was filtered to in the Asset Browser",
        "Failed to filter to the expected asset type"
    )


def AssetBrowser_SearchFiltering():

    import pyside_utils

    @pyside_utils.wrap_async
    async def run_test():
        """
        Summary:
        Asset Browser - Filtering

        Expected Behavior:
        The file tree is expanded and the list of assets is reduced while typing to eventually show only the filtered asset.
        The search bar is cleared.
        The file tree is reduced upon selecting the filter.
        The file tree is increased upon selecting additional file types.
        The file tree is reduced upon removing the file type filter.
        The file tree is reset and all folders are displayed.

        Test Steps:
         1) Open level
         2) Open Asset Browser
         3) Type the name of an asset in the search bar and make sure only one asset is filtered in Asset browser
         4) Click the "X" in the search bar.
         5) Select an asset type to filter by (Animation)
         6) Add additional filter(FileTag) from the filter menu
         7) Remove one of the filtered asset types from the list of applied filters
         8) Remove all of the filter asset types from the list of filters


        Note:
        - This test file must be called from the Open 3D Engine Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """

        from PySide2 import QtWidgets, QtTest, QtCore
        from PySide2.QtCore import Qt

        import azlmbr.legacy.general as general

        import editor_python_test_tools.hydra_editor_utils as hydra
        from editor_python_test_tools.utils import Report

        def verify_files_appeared(model, allowed_asset_extensions, parent_index=QtCore.QModelIndex()):
            indexes = [parent_index]
            while len(indexes) > 0:
                parent_index = indexes.pop(0)
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
                    indexes.append(cur_index)
            return True

        # 1) Open an existing simple level
        hydra.open_base_level()

        # 2) Open Asset Browser (if not opened already)
        editor_window = pyside_utils.get_editor_main_window()
        asset_browser_open = general.is_pane_visible("Asset Browser")
        if not asset_browser_open:
            Report.info("Opening Asset Browser")
            action = pyside_utils.get_action_for_menu_path(editor_window, "Tools", "Asset Browser")
            action.trigger()
        else:
            Report.info("Asset Browser is already open")
        editor_window = pyside_utils.get_editor_main_window()
        app = QtWidgets.QApplication.instance()

        # 3) Switch to list view, type the name of an asset in the search bar and make sure it is filtered to and selectable
        asset_browser = editor_window.findChild(QtWidgets.QDockWidget, "Asset Browser")
        tree_view_button = asset_browser.findChild(QtWidgets.QToolButton, "m_treeViewButton")
        tree_view_button.click()
        general.idle_wait(1.0)
        search_bar = asset_browser.findChild(QtWidgets.QLineEdit, "textSearch")

        # Add a small pause when typing in the search bar in order to check that the entries are updated properly
        search_bar.setText("Cedar.f")
        general.idle_wait(0.5)
        search_bar.setText("Cedar.fbx")
        general.idle_wait(0.5)

        asset_browser_tree = asset_browser.findChild(QtWidgets.QTreeView, "m_assetBrowserTreeViewWidget")
        found = await pyside_utils.wait_for_condition(lambda: pyside_utils.find_child_by_pattern(asset_browser_tree, "cedar.fbx"), 5.0)
        if found:
            model_index = pyside_utils.find_child_by_pattern(asset_browser_tree, "cedar.fbx")
        else:
            Report.result(Tests.asset_filtered, found)
        asset_browser_tree.scrollTo(model_index)
        pyside_utils.item_view_index_mouse_click(asset_browser_tree, model_index)
        is_filtered = await pyside_utils.wait_for_condition(
            lambda: asset_browser_tree.currentIndex() == model_index, 5.0)
        Report.result(Tests.asset_filtered, is_filtered)

        # 4) Click the "X" in the search bar.
        clear_search = asset_browser.findChild(QtWidgets.QToolButton, "ClearToolButton")
        clear_search.click()
        
        # 5) Select an asset type to filter by (Animation)
        tool_button = asset_browser.findChild(QtWidgets.QToolButton, "assetTypeSelector")
        pyside_utils.click_button_async(tool_button)
        line_edit = tool_button.findChildren(QtWidgets.QLineEdit, "filteredSearchWidget")[0]
        line_edit.setText("Animation")
        tree = tool_button.findChildren(QtWidgets.QTreeView)[0]
        animation_model_index = await pyside_utils.wait_for_child_by_pattern(tree, "Animation")
        tree.model().setData(animation_model_index, 2, Qt.CheckStateRole)
        general.idle_wait(1.0)
        # check asset types after clicking on Animation filter
        asset_type_filter = verify_files_appeared(asset_browser_tree.model(), ["i_caf", "fbx", "xml", "animgraph", "motionset", "actor", "motion"])
        Report.result(Tests.asset_type_filtered, asset_type_filter)

        # 6) Add additional filter(FileTag) from the filter menu
        line_edit.setText("FileTag")
        filetag_model_index = await pyside_utils.wait_for_child_by_pattern(tree, "FileTag")
        tree.model().setData(filetag_model_index, 2, Qt.CheckStateRole)
        general.idle_wait(1.0)
        # check asset types after clicking on FileTag filter
        more_types_filtered = verify_files_appeared(
                asset_browser_tree.model(), ["i_caf", "fbx", "xml", "animgraph", "motionset", "filetag", "actor", "motion"]
        )
        Report.result(Tests.asset_type_filtered, more_types_filtered)

        # 7) Remove one of the filtered asset types from the list of applied filters
        filter_layout = asset_browser.findChild(QtWidgets.QFrame, "containerLayout")
        
        animation_close_button = filter_layout.children()[4]
        first_close_button = animation_close_button.findChild(QtWidgets.QPushButton, "closeTag")
        first_close_button.click()
        general.idle_wait(1.0)
        # check asset types after removing Animation filter
        remove_filtered = verify_files_appeared(asset_browser_tree.model(), ["filetag"])
        Report.result(Tests.asset_type_filtered, remove_filtered)

        # 8) Remove all of the filter asset types from the list of filters
        filetag_close_button = filter_layout.children()[3]
        second_close_button = filetag_close_button.findChild(QtWidgets.QPushButton, "closeTag")
        second_close_button.click()

        # Click off of the Asset Browser filter window to close it
        QtTest.QTest.mouseClick(tree, Qt.LeftButton, Qt.NoModifier)

        # 9) Restore Asset Browser tool state and
        if not asset_browser_open:
            Report.info("Closing Asset Browser")
            general.close_pane("Asset Browser")

    run_test()


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(AssetBrowser_SearchFiltering)
