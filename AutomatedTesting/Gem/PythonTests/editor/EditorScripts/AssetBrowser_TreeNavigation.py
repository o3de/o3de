"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

"""
C13660195: Asset Browser - File Tree Navigation
"""

import os
import sys
from PySide2 import QtWidgets, QtTest, QtCore

import azlmbr.legacy.general as general
import azlmbr.paths

sys.path.append(os.path.join(azlmbr.paths.devroot, 'AutomatedTesting', 'Gem', 'PythonTests'))
from editor_python_test_tools.editor_test_helper import EditorTestHelper
import editor_python_test_tools.pyside_utils as pyside_utils


class AssetBrowserTreeNavigationTest(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="AssetBrowser_TreeNavigation", args=["level"])

    def run_test(self):
        """
        Summary:
        Verify if we are able to expand a file hierarchy in the Asset Browser and ScrollBar appears
        appropriately.

        Expected Behavior:
        The folder list is expanded to display the children of the selected folder.
        A scroll bar appears to allow scrolling up and down through the asset browser.
        Assets are present in the Asset Browser.

        Test Steps:
        1) Open a new level
        2) Open Asset Browser
        3) Collapse all files initially
        4) Get all Model Indexes
        5) Expand each of the folder and verify if it is opened
        6) Verify if the ScrollBar appears after expanding the tree

        Note:
        - This test file must be called from the Open 3D Engine Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """

        def collapse_expand_and_verify(model_index, hierarchy_level):
            tree.collapse(model_index)
            collapse_success = not tree.isExpanded(model_index)
            self.log(f"Level {hierarchy_level} collapsed: {collapse_success}")
            tree.expand(model_index)
            expand_success = tree.isExpanded(model_index)
            self.log(f"Level {hierarchy_level} expanded: {expand_success}")
            return collapse_success and expand_success

        # This is the hierarchy we are expanding (4 steps inside)
        self.file_path = ("AutomatedTesting", "Assets", "ImageGradients", "image_grad_test_gsi.png")

        # 1) Open a new level
        self.test_success = self.create_level(
            self.args["level"],
            heightmap_resolution=1024,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=4096,
            use_terrain=False,
        )

        # 2) Open Asset Browser (if not opened already)
        editor_window = pyside_utils.get_editor_main_window()
        asset_browser_open = general.is_pane_visible("Asset Browser")
        if not asset_browser_open:
            self.log("Opening Asset Browser")
            action = pyside_utils.get_action_for_menu_path(editor_window, "Tools", "Asset Browser")
            action.trigger()
        else:
            self.log("Asset Browser is already open")

        # 3) Collapse all files initially
        main_window = editor_window.findChild(QtWidgets.QMainWindow)
        asset_browser = pyside_utils.find_child_by_hierarchy(main_window, ..., "Asset Browser")
        tree = pyside_utils.find_child_by_hierarchy(asset_browser, ..., "m_assetBrowserTreeViewWidget")
        scroll_area = tree.findChild(QtWidgets.QWidget, "qt_scrollarea_vcontainer")
        scroll_bar = scroll_area.findChild(QtWidgets.QScrollBar)
        tree.collapseAll()

        # 4) Get all Model Indexes
        model_index_1 = pyside_utils.find_child_by_hierarchy(tree, self.file_path[0])
        model_index_2 = pyside_utils.find_child_by_hierarchy(model_index_1, self.file_path[1])
        model_index_3 = pyside_utils.find_child_by_hierarchy(model_index_2, self.file_path[2])
        model_index_4 = pyside_utils.find_child_by_hierarchy(model_index_3, self.file_path[3])

        # 5) Verify each level of the hierarchy to the file can be collapsed/expanded
        self.test_success = collapse_expand_and_verify(model_index_1, 1) and self.test_success
        self.test_success = collapse_expand_and_verify(model_index_2, 2) and self.test_success
        self.test_success = collapse_expand_and_verify(model_index_3, 3) and self.test_success
        self.log(f"Collapse/Expand tests: {self.test_success}")

        # Select the asset
        tree.scrollTo(model_index_4)
        pyside_utils.item_view_index_mouse_click(tree, model_index_4)

        # Verify if the currently selected item model index is same as the Asset Model index
        # to prove that it is visible
        asset_visible = tree.currentIndex() == model_index_4
        self.test_success = asset_visible and self.test_success
        self.log(f"Asset visibility test: {asset_visible}")

        # 6) Verify if the ScrollBar appears after expanding the tree
        scrollbar_visible = scroll_bar.isVisible()
        self.test_success = scrollbar_visible and self.test_success
        self.log(f"Scrollbar visibility test: {scrollbar_visible}")

        # 7) Restore Asset Browser tool state
        if not asset_browser_open:
            self.log("Closing Asset Browser")
            general.close_pane("Asset Browser")


test = AssetBrowserTreeNavigationTest()
test.run()
