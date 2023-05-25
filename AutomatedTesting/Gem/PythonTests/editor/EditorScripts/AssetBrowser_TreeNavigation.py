"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    collapse_expand = (
        "Asset Browser hierarchy successfully collapsed/expanded",
        "Failed to collapse/expand Asset Browser hierarchy"
    )
    asset_visible = (
        "Expected asset is visible in the Asset Browser hierarchy",
        "Failed to find expected asset in the Asset Browser hierarchy"
    )
    scrollbar_visible = (
        "Scrollbar is visible",
        "Scrollbar was not found"
    )


def AssetBrowser_TreeNavigation():
    """
    Summary:
    Verify if we are able to expand a file hierarchy in the Asset Browser and ScrollBar appears
    appropriately.

    Expected Behavior:
    The folder list is expanded to display the children of the selected folder.
    A scroll bar appears to allow scrolling up and down through the asset browser.
    Assets are present in the Asset Browser.

    Test Steps:
    1) Open a simple level
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

    from PySide2 import QtWidgets, QtTest, QtCore

    import azlmbr.legacy.general as general

    import pyside_utils
    import editor_python_test_tools.hydra_editor_utils as hydra
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    def collapse_expand_and_verify(model_index, hierarchy_level):
        tree.collapse(model_index)
        collapse_success = not tree.isExpanded(model_index)
        Report.info(f"Level {hierarchy_level} collapsed: {collapse_success}")
        tree.expand(model_index)
        expand_success = tree.isExpanded(model_index)
        Report.info(f"Level {hierarchy_level} expanded: {expand_success}")
        return collapse_success and expand_success

    # This is the hierarchy we are expanding (4 steps inside)
    file_path = ("AutomatedTesting", "Assets", "ImageGradients", "image_grad_test_gsi.png")

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

    # 3) Switch to List view and collapse all files initially
    main_window = editor_window.findChild(QtWidgets.QMainWindow)
    asset_browser = pyside_utils.find_child_by_pattern(main_window, text="Asset Browser", type=QtWidgets.QDockWidget)
    tree_view_button = asset_browser.findChild(QtWidgets.QToolButton, "m_treeViewButton")
    tree_view_button.click()
    general.idle_wait(1.0)
    tree = pyside_utils.find_child_by_pattern(asset_browser, "m_assetBrowserTreeViewWidget")
    scroll_area = tree.findChild(QtWidgets.QWidget, "qt_scrollarea_vcontainer")
    scroll_bar = scroll_area.findChild(QtWidgets.QScrollBar)
    tree.collapseAll()

    # 4) Get all Model Indexes
    model_index_1 = pyside_utils.find_child_by_hierarchy(tree, file_path[0])
    model_index_2 = pyside_utils.find_child_by_hierarchy(model_index_1, file_path[1])
    model_index_3 = pyside_utils.find_child_by_hierarchy(model_index_2, file_path[2])
    model_index_4 = pyside_utils.find_child_by_hierarchy(model_index_3, file_path[3])

    # 5) Verify each level of the hierarchy to the file can be collapsed/expanded
    Report.result(Tests.collapse_expand, collapse_expand_and_verify(model_index_1, 1) and
                  collapse_expand_and_verify(model_index_2, 2) and collapse_expand_and_verify(model_index_3, 3))

    # Select the asset
    tree.scrollTo(model_index_4)
    pyside_utils.item_view_index_mouse_click(tree, model_index_4)

    # Verify if the currently selected item model index is same as the Asset Model index
    # to prove that it is visible
    Report.result(Tests.asset_visible, tree.currentIndex() == model_index_4)

    # 6) Verify if the ScrollBar appears after expanding the tree
    Report.result(Tests.scrollbar_visible, scroll_bar.isVisible())

    # 7) Restore Asset Browser tool state
    if not asset_browser_open:
        Report.info("Closing Asset Browser")
        general.close_pane("Asset Browser")


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(AssetBrowser_TreeNavigation)
