"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# fmt: off
class Tests():
    pane_open       = ("Script Canvas pane successfully opened", "Script Canvas pane failed to open")
    click_expand    = ("Category expanded on left click",        "Category failed to expand on left click")
    click_collapse  = ("Category collapsed on left click",       "Category failed to collapse on left click")
    dClick_expand   = ("Category expanded on double click",      "Category failed to expand on double click")
    dClick_collapse = ("Category collapsed on double click",     "Category failed to collapse on double click")
# fmt: on


def NodeCategory_ExpandOnClick():
    """
    Summary:
     Verifying the expand/collapse functionality on node categories

    Expected Behavior:
     The node category should expand when double clicked or when the drop down indicator is
     left-clicked. Once expanded, it should be collapsed via the same actions.

    Test Steps:
      1) Open Script Canvas pane
      2) Get the SC window objects
      3) Ensure all categories are collapsed for a clean state
      4) Left-Click on a node category arrow to expand it
      5) Verify it expanded
      6) Left-Click on a node category arrow to collapse it
      7) Verify it collapsed
      8) Double-Click on a node category to expand it
      9) Verify it expanded
     10) Double-Click on a node category to collapse it
     11) Verify it collapsed

    Note:
     - This test file must be called from the Open 3D Engine Editor command terminal
     - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """
    from utils import Report
    from PySide2 import QtCore, QtWidgets, QtTest
    from PySide2.QtTest import QTest
    import pyside_utils
    import azlmbr.legacy.general as general

    def left_click_arrow(item_view, index):
        original_state = item_view.isExpanded(index)
        rect_center_y = item_view.visualRect(index).center().y()
        rect_left_x = item_view.visualRect(index).left()
        for i in range(5):  # this range can be increased for safe side
            QtTest.QTest.mouseClick(
                item_view.viewport(),
                QtCore.Qt.LeftButton,
                QtCore.Qt.NoModifier,
                QtCore.QPoint(rect_left_x - i, rect_center_y),
            )
            if item_view.isExpanded(index) != original_state:
                break

    def double_click(item_view, index):
        item_index_center = item_view.visualRect(index).center()
        # Left click on the item before trying to double click, will otherwise fail to expand
        # as first click would highlight and second click would be a 'single click'
        pyside_utils.item_view_index_mouse_click(item_view, index)
        QTest.mouseDClick(item_view.viewport(), QtCore.Qt.LeftButton, QtCore.Qt.NoModifier, item_index_center)

    # 1) Open Script Canvas pane
    general.open_pane("Script Canvas")
    Report.critical_result(Tests.pane_open, general.is_pane_visible("Script Canvas"))

    # 2) Get the SC window objects
    editor_window = pyside_utils.get_editor_main_window()
    sc = editor_window.findChild(QtWidgets.QDockWidget, "Script Canvas")
    if sc.findChild(QtWidgets.QDockWidget, "NodePalette") is None:
        action = pyside_utils.find_child_by_pattern(sc, {"text": "Node Palette", "type": QtWidgets.QAction})
        action.trigger()
    node_palette = sc.findChild(QtWidgets.QDockWidget, "NodePalette")
    nodeTree = node_palette.findChild(QtWidgets.QTreeView, "treeView")
    ai_index = pyside_utils.find_child_by_pattern(nodeTree, "AI")

    # 3) Ensure all categories are collapsed for a clean state
    nodeTree.collapseAll()

    # 4) Left-Click on a node category arrow to expand it
    left_click_arrow(nodeTree, ai_index)

    # 5) Verify it expanded
    Report.result(Tests.click_expand, nodeTree.isExpanded(ai_index))

    # 6) Left-Click on a node category arrow to collapse it
    left_click_arrow(nodeTree, ai_index)

    # 7) Verify it collapsed
    Report.result(Tests.click_collapse, not nodeTree.isExpanded(ai_index))

    # 8) Double-Click on a node category to expand it
    double_click(nodeTree, ai_index)

    # 9) Verify it expanded
    Report.result(Tests.dClick_expand, nodeTree.isExpanded(ai_index))

    # 10) Double-Click on a node category to collapse it
    double_click(nodeTree, ai_index)

    # 11) Verify it collapsed
    Report.result(Tests.dClick_collapse, not nodeTree.isExpanded(ai_index))


if __name__ == "__main__":
    import ImportPathHelper as imports

    imports.init()
    from utils import Report

    Report.start_test(NodeCategory_ExpandOnClick)
