"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from PySide2 import QtWidgets, QtTest, QtCore
import pyside_utils
from editor_python_test_tools.utils import TestHelper as helper
from editor_python_test_tools.utils import Report
import azlmbr.legacy.general as general
import scripting_utils.scripting_tools as scripting_tools
from scripting_utils.scripting_constants import WAIT_TIME_3, SCRIPT_CANVAS_UI, NODE_PALETTE_UI, NODE_PALETTE_QT,\
    TREE_VIEW_QT, NODE_CATEGORY_MATH


# fmt: off
class Tests:
    pane_close      = ("Script Canvas pane successfully closed", "Script Canvas pane failed to close")
    pane_open       = ("Script Canvas pane successfully opened", "Script Canvas pane failed to open")
    click_expand    = ("Category expanded on left click",        "Category failed to expand on left click")
    click_collapse  = ("Category collapsed on left click",       "Category failed to collapse on left click")
    dClick_expand   = ("Category expanded on double click",      "Category failed to expand on double click")
    dClick_collapse = ("Category collapsed on double click",     "Category failed to collapse on double click")
# fmt: on


class NodeCategory_ExpandOnClick:
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
      4) Left-Click on a node category arrow to expand it and verify it's expanded
      5) Left-Click on a node category arrow to collapse it and verify it's collapsed
      6) Double-Click on a node category to expand it then verify it's expanded
      7) Double-Click on a node category to collapse it and verify it's collapsed


    Note:
     - This test file must be called from the Open 3D Engine Editor command terminal
     - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    def __init__(self):
        self.editor_main_window = None
        self.sc_editor = None
        self.sc_editor_main_window = None


    def left_click_expander_button(self, node_palette_node_tree, category_index):
        rect_left_x = 1 # 1 pixel from the left side of the widget looks like where the expand button begins
        rect_center_y = node_palette_node_tree.visualRect(category_index).center().y()
        click_position = QtCore.QPoint(rect_left_x, rect_center_y)
        # click position relative to node palette tree view and not screen space xy
        QtTest.QTest.mouseClick(node_palette_node_tree.viewport(),
                                QtCore.Qt.LeftButton,
                                QtCore.Qt.NoModifier,
                                click_position,
                                )

    def double_click_node_category(self, node_palette_node_tree, index):
        item_index_center = node_palette_node_tree.visualRect(index).center()
        # Left click on the item before trying to double click, will otherwise fail to expand
        # as first click would highlight and second click would be a 'single click'
        pyside_utils.item_view_index_mouse_click(node_palette_node_tree, index)
        QtTest.QTest.mouseDClick(node_palette_node_tree.viewport(),
                                 QtCore.Qt.LeftButton,
                                 QtCore.Qt.NoModifier,
                                 item_index_center)


    def wait_and_verify_category_expanded_state(self, test_case, node_palette_node_tree, node_palette_math_category, expanded = True):
        category_has_expanded = helper.wait_for_condition(
            lambda: node_palette_node_tree.isExpanded(node_palette_math_category), WAIT_TIME_3)
        Report.result(test_case, expanded == category_has_expanded)

    @pyside_utils.wrap_async
    async def run_test(self):

        # Preconditions
        general.idle_enable(True)
        general.close_pane(SCRIPT_CANVAS_UI)
        helper.wait_for_condition(lambda: general.is_pane_visible(SCRIPT_CANVAS_UI) is None, WAIT_TIME_3)

        # 1) Open Script Canvas pane
        general.open_pane(SCRIPT_CANVAS_UI)
        Report.critical_result(Tests.pane_open, general.is_pane_visible(SCRIPT_CANVAS_UI))

        # 2) Get the SC window objects (editor, sc editor, node palette elements)
        scripting_tools.initialize_editor_object(self)
        scripting_tools.initialize_sc_editor_objects(self)
        scripting_tools.open_node_palette(self)

        # wait for the node palette and other SC elements to render

        helper.wait_for_condition(lambda: self.sc_editor.findChild(QtWidgets.QDockWidget, NODE_PALETTE_QT) is not None, WAIT_TIME_3)
        node_palette_node_tree = scripting_tools.get_node_palette_node_tree_qt_object(self)
        node_palette_math_category = scripting_tools.get_node_palette_category_qt_object(self, NODE_CATEGORY_MATH)

        # 3) Ensure all categories are collapsed for a clean state
        node_palette_node_tree.collapseAll()

        # 4) Left-Click on a node category arrow to expand it and verify it's expanded
        self.left_click_expander_button(node_palette_node_tree, node_palette_math_category)
        self.wait_and_verify_category_expanded_state(Tests.click_expand, node_palette_node_tree, node_palette_math_category)

        # 5) Left-Click on a node category arrow to collapse it and verify it's collapsed
        self.left_click_expander_button(node_palette_node_tree, node_palette_math_category)
        self.wait_and_verify_category_expanded_state(Tests.click_collapse, node_palette_node_tree, node_palette_math_category, False)

        # 6) Double-Click on a node category to expand it then verify it's expanded
        self.double_click_node_category(node_palette_node_tree, node_palette_math_category)
        self.wait_and_verify_category_expanded_state(Tests.dClick_expand, node_palette_node_tree, node_palette_math_category)

        # 7) Double-Click on a node category to collapse it and verify it's collapsed
        self.double_click_node_category(node_palette_node_tree, node_palette_math_category)
        self.wait_and_verify_category_expanded_state(Tests.dClick_collapse, node_palette_node_tree, node_palette_math_category, False)


test = NodeCategory_ExpandOnClick()
test.run_test()

