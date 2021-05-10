"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.


Test case ID: T92568940
Test Case Title: Categories and Nodes can be selected
Test case ID: 92568940
"""


# fmt: off
class Tests():
    category_selected  = ("Category can be selected",  "Category cannot be selected")
    node_selected      = ("Node can be selected",      "Node cannot be selected")
# fmt: on


GENERAL_WAIT = 0.5  # seconds


def NodePalette_SelectNode():
    """
    Summary:
     Categories and Nodes can be selected

    Expected Behavior:
     When clicked on Node Palette, nodes and categories can be selected.

    Test Steps:
     1) Open Script Canvas window (Tools > Script Canvas)
     2) Get the SC window object
     3) Expand QTreeView
     4) Click on category and check if it is selected
     5) Click on node and check if it is selected
     6) Close Script Canvas window


    Note:
     - This test file must be called from the Open 3D Engine Editor command terminal
     - Any passed and failed tests are written to the Editor.log file.
        Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    CATEGORY = "AI"
    NODE = "Find Path To Entity"

    from PySide2 import QtWidgets
    import azlmbr.legacy.general as general

    import pyside_utils
    from utils import TestHelper as helper

    # 1) Open Script Canvas window (Tools > Script Canvas)
    general.idle_enable(True)
    general.open_pane("Script Canvas")
    helper.wait_for_condition(lambda: general.is_pane_visible("Script Canvas"), 5.0)

    # 2) Get the SC window object
    editor_window = pyside_utils.get_editor_main_window()
    sc = editor_window.findChild(QtWidgets.QDockWidget, "Script Canvas")
    if sc.findChild(QtWidgets.QDockWidget, "NodePalette") is None:
        action = pyside_utils.find_child_by_pattern(sc, {"text": "Node Palette", "type": QtWidgets.QAction})
        action.trigger()
    node_palette = sc.findChild(QtWidgets.QDockWidget, "NodePalette")
    tree = node_palette.findChild(QtWidgets.QTreeView, "treeView")

    # 3) Expand QTreeView
    tree.expandAll()

    # 4) Click on category and check if it is selected
    category_index = pyside_utils.find_child_by_hierarchy(tree, CATEGORY)
    tree.scrollTo(category_index)
    pyside_utils.item_view_index_mouse_click(tree, category_index)
    pyside_utils.wait_for_condition(tree.selectedIndexes() and tree.selectedIndexes()[0] == category_index)
    Report.result(Tests.category_selected, tree.selectedIndexes()[0] == category_index)

    # 5) Click on node and check if it is selected
    node_index = pyside_utils.find_child_by_pattern(tree, NODE)
    helper.wait_for_condition(lambda: tree.isExpanded(node_index), GENERAL_WAIT)
    pyside_utils.item_view_index_mouse_click(tree, node_index)
    pyside_utils.wait_for_condition(tree.selectedIndexes()[0] == node_index)
    Report.result(Tests.node_selected, tree.selectedIndexes()[0] == node_index)

    # 6) Close Script Canvas window
    general.close_pane("Script Canvas")


if __name__ == "__main__":
    import ImportPathHelper as imports

    imports.init()

    from utils import Report

    Report.start_test(NodePalette_SelectNode)
