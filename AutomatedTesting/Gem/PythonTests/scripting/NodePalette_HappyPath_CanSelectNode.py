"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    category_selected = ("Category can be selected", "Category cannot be selected")
    node_selected = ("Node can be selected", "Node cannot be selected")

def TestNodePaletteHappyPathCanSelectNode():


    """
        Summary:
         Categories and Nodes can be selected

        Expected Behavior:
         A category can be selected inside the Node Palette
         A Node can be selected inside the Node Palette

        Test Steps:
         1) Open Script Canvas window (Tools > Script Canvas)
         2) Get the SC window object and get a handle on the Node Palette
         3) Get a handle on the node palette's tree of nodes then expand the QTView tree object
         4) Scroll down to the category we are looking for and verify we can click on it
         5) Scroll down to the node within the category and verify we can click on it
         6) Close Script Canvas window


        Note:
         - This test file must be called from the Open 3D Engine Editor command terminal
         - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """

    # Pre conditions
    from PySide2 import QtWidgets as qtwidgets
    from editor_python_test_tools.utils import Report
    import azlmbr.legacy.general as general
    import pyside_utils
    from scripting_utils.scripting_constants import (SCRIPT_CANVAS_UI, NODE_PALETTE_UI, WAIT_TIME_3, TREE_VIEW_QT,
                                                     NODE_PALETTE_QT, NODE_CATEGORY_MATH, NODE_STRING_TO_NUMBER)

    general.idle_enable(True)

    # 1) Open Script Canvas window (Tools > Script Canvas)
    general.open_pane(SCRIPT_CANVAS_UI)
    pyside_utils.wait_for_condition(lambda: general.is_pane_visible(SCRIPT_CANVAS_UI), WAIT_TIME_3)

    # 2) Get the SC window object and get a handle on the Node Palette
    editor_window = pyside_utils.get_editor_main_window()
    sc = editor_window.findChild(qtwidgets.QDockWidget, SCRIPT_CANVAS_UI)
    if sc.findChild(qtwidgets.QDockWidget, NODE_PALETTE_QT) is None:
        action = pyside_utils.find_child_by_pattern(sc, {"text": NODE_PALETTE_UI,
                                                         "type": qtwidgets.QAction})
        action.trigger()
    node_palette = sc.findChild(qtwidgets.QDockWidget, NODE_PALETTE_QT)

    # 3) Get a handle on the node palette's tree of nodes then expand the QTView tree object
    node_palette_tree_view = node_palette.findChild(qtwidgets.QTreeView, TREE_VIEW_QT)
    node_palette_tree_view.expandAll()

    # 4) Scroll down to the category we are looking for and verify we can click on it
    category_index = pyside_utils.find_child_by_hierarchy(node_palette_tree_view, NODE_CATEGORY_MATH)
    node_palette_tree_view.scrollTo(category_index)
    pyside_utils.item_view_index_mouse_click(node_palette_tree_view, category_index)
    pyside_utils.wait_for_condition(lambda: node_palette_tree_view.selectedIndexes() and
                                            (node_palette_tree_view.selectedIndexes()[0] == category_index),
                                    WAIT_TIME_3)
    Report.result(Tests.category_selected, node_palette_tree_view.selectedIndexes()[0] == category_index)

    # 5) Scroll down to the node within the category and verify we can click on it
    pyside_utils.find_child_by_pattern(node_palette_tree_view, NODE_STRING_TO_NUMBER)
    node_index = pyside_utils.find_child_by_pattern(node_palette_tree_view, NODE_STRING_TO_NUMBER)
    node_palette_tree_view.scrollTo(node_index)
    pyside_utils.item_view_index_mouse_click(node_palette_tree_view, node_index)
    pyside_utils.wait_for_condition(lambda: node_palette_tree_view.selectedIndexes() and
                                            (node_palette_tree_view.selectedIndexes()[0] == node_index),
                                    WAIT_TIME_3)
    Report.result(Tests.node_selected, node_palette_tree_view.selectedIndexes()[0] == node_index)

    # 6) Close Script Canvas window
    general.close_pane(SCRIPT_CANVAS_UI)


if __name__ == "__main__":
    import ImportPathHelper as imports

    imports.init()
    from utils import Report

    Report.start_test(TestNodePaletteHappyPathCanSelectNode)
    