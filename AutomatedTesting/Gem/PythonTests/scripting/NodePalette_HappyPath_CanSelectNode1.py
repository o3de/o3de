"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from PySide2 import QtWidgets
from editor_python_test_tools.utils import TestHelper as helper
from editor_python_test_tools.utils import Report
import azlmbr.legacy.general as general
import editor_python_test_tools.pyside_utils as pyside_utils

class Tests():
    category_selected = ("Category can be selected", "Category cannot be selected")
    node_selected     = ("Node can be selected",     "Node cannot be selected")

TIME_TO_WAIT = 3  # seconds
NODE_CATEGORY = "Math"
NODE_NAME = "String To Number"
    
class TestNodePaletteHappyPathCanSelectNode:
    """
    Summary:
     Categories and Nodes can be selected

    Expected Behavior:
     A category can be selected inside the Node Palette
     A Node can be selected inside the Node Palette

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
    
    @pyside_utils.wrap_async
    async def run_test(self):
    
        # Pre conditions
        general.idle_enable(True)
     
        # 1) Open Script Canvas window (Tools > Script Canvas)
        general.open_pane("Script Canvas")
        helper.wait_for_condition(lambda: general.is_pane_visible("Script Canvas"), TIME_TO_WAIT)

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
        category_index = pyside_utils.find_child_by_hierarchy(tree, NODE_CATEGORY)
        tree.scrollTo(category_index)
        pyside_utils.item_view_index_mouse_click(tree, category_index)
        #ths was a pyside_utils.wait_for_condition. I don't think it needed to be changed
        helper.wait_for_condition(lambda: tree.selectedIndexes() and tree.selectedIndexes()[0] == category_index, TIME_TO_WAIT)
        Report.result(Tests.category_selected, tree.selectedIndexes()[0] == category_index)

        # 5) Click on node and check if it is selected
        node_index = pyside_utils.find_child_by_pattern(tree, NODE_NAME)
        helper.wait_for_condition(lambda: tree.isExpanded(node_index), TIME_TO_WAIT)
        pyside_utils.item_view_index_mouse_click(tree, node_index)
        helper.wait_for_condition(lambda: tree.selectedIndexes()[0] == node_index, TIME_TO_WAIT)
        Report.result(Tests.node_selected, tree.selectedIndexes()[0] == node_index)

        # 6) Close Script Canvas window
        general.close_pane("Script Canvas")

test = TestNodePaletteHappyPathCanSelectNode()
test.run_test()