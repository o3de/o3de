"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

"""
C13660194 : Asset Browser - Filtering
"""

import os
import sys
from PySide2 import QtWidgets, QtTest, QtCore
from PySide2.QtCore import Qt

import azlmbr.legacy.general as general
import azlmbr.paths

sys.path.append(os.path.join(azlmbr.paths.devroot, 'AutomatedTesting', 'Gem', 'PythonTests'))
import editor_python_test_tools.hydra_editor_utils as hydra
import editor_python_test_tools.pyside_utils as pyside_utils
from editor_python_test_tools.editor_test_helper import EditorTestHelper


class AssetBrowserSearchFilteringTest(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="AssetBrowser_SearchFiltering", args=["level"])

    @pyside_utils.wrap_async
    async def run_test(self):
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

        self.incorrect_file_found = False

        def verify_files_appeared(model, allowed_asset_extentions, parent_index=QtCore.QModelIndex()):
            indexes = [parent_index]
            while len(indexes) > 0:
                parent_index = indexes.pop(0)
                for row in range(model.rowCount(parent_index)):
                    cur_index = model.index(row, 0, parent_index)
                    cur_data = cur_index.data(Qt.DisplayRole)
                    if (
                        "." in cur_data
                        and (cur_data.lower().split(".")[-1] not in allowed_asset_extentions)
                        and not cur_data[-1] == ")"
                    ):
                        print(f"Incorrect file found: {cur_data}")
                        self.incorrect_file_found = True
                        indexes = list()
                        break
                    indexes.append(cur_index)


        # 1) Open level
        self.test_success = self.create_level(
            self.args["level"],
            heightmap_resolution=1024,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=4096,
            use_terrain=False,
        )

        # 2) Open Asset Browser
        general.close_pane("Asset Browser")
        general.open_pane("Asset Browser")
        editor_window = pyside_utils.get_editor_main_window()
        app = QtWidgets.QApplication.instance()
        
        # 3) Type the name of an asset in the search bar and make sure only one asset is filtered in Asset browser
        asset_browser = editor_window.findChild(QtWidgets.QDockWidget, "Asset Browser")
        search_bar = asset_browser.findChild(QtWidgets.QLineEdit, "textSearch")
        search_bar.setText("cedar.fbx")
        asset_browser_tree = asset_browser.findChild(QtWidgets.QTreeView, "m_assetBrowserTreeViewWidget")
        model_index = pyside_utils.find_child_by_pattern(asset_browser_tree, "cedar.fbx")
        pyside_utils.item_view_index_mouse_click(asset_browser_tree, model_index)
        is_filtered = pyside_utils.wait_for_condition(
            lambda: asset_browser_tree.indexBelow(asset_browser_tree.currentIndex()) == QtCore.QModelIndex(), 5.0)
        if is_filtered:
            print("cedar.fbx asset is filtered in Asset Browser")

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
        verify_files_appeared(asset_browser_tree.model(), ["i_caf", "fbx", "xml", "animgraph", "motionset"])
        print(f"Animation file type(s) is present in the file tree: {not self.incorrect_file_found}")

        # 6) Add additional filter(FileTag) from the filter menu
        self.incorrect_file_found = False
        line_edit.setText("FileTag")
        filetag_model_index = await pyside_utils.wait_for_child_by_pattern(tree, "FileTag")
        tree.model().setData(filetag_model_index, 2, Qt.CheckStateRole)
        general.idle_wait(1.0)
        # check asset types after clicking on FileTag filter
        verify_files_appeared(
                asset_browser_tree.model(), ["i_caf", "fbx", "xml", "animgraph", "motionset", "filetag"]
        )
        print(f"FileTag file type(s) and Animation file type(s) is present in the file tree: {not self.incorrect_file_found}")

        # 7) Remove one of the filtered asset types from the list of applied filters
        self.incorrect_file_found = False
        filter_layout = asset_browser.findChild(QtWidgets.QFrame, "filteredLayout")
        animation_close_button = filter_layout.children()[1]
        first_close_button = animation_close_button.findChild(QtWidgets.QPushButton, "closeTag")
        first_close_button.click()
        general.idle_wait(1.0)
        # check asset types after removing Animation filter
        verify_files_appeared(asset_browser_tree.model(), ["filetag"])
        print(f"FileTag file type(s) is present in the file tree after removing Animation filter: {not self.incorrect_file_found}")

        # 8) Remove all of the filter asset types from the list of filters
        filetag_close_button = filter_layout.children()[1]
        second_close_button = filetag_close_button.findChild(QtWidgets.QPushButton, "closeTag")
        second_close_button.click()

        # 9) Close the asset browser
        asset_browser.close()


test = AssetBrowserSearchFilteringTest()
test.run()
