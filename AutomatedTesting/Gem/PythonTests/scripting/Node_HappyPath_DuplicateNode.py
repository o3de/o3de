"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from PySide2 import QtWidgets, QtTest
from PySide2.QtCore import Qt
from editor_python_test_tools.utils import Report
from editor_python_test_tools.utils import TestHelper as helper
import pyside_utils
import azlmbr.legacy.general as general
import scripting_utils.scripting_tools as scripting_tools
from scripting_utils.scripting_constants import (SCRIPT_CANVAS_UI, NODE_INSPECTOR_QT, NODE_INSPECTOR_UI,
                                                 WAIT_TIME_3, WAIT_FRAMES)


COMMAND_LINE_ARGS = "add_node Print"
EXPECTED_NODE_TITLE_STRING = "Print - Utilities/Debug (2 Selected)"


# fmt: off
class Tests:
    node_duplicated = ("Successfully duplicated node", "Failed to duplicate the node")
# fmt: on


class Node_HappyPath_DuplicateNode:
    """
    Summary:
     Duplicating node in graph

    Expected Behavior:
     Upon selecting a node and pressing Ctrl+D, the node will be duplicated

    Test Steps:
     1) Open Script Canvas window (Tools > Script Canvas) and initialize Qt objects
     2) Open a new graph
     3) Add a node to the graph by emulating the Command Line tool's workflow
     4) Duplicate nodes on graph w/ ctrl+a and ctrl+d keystroke
     5) Verify the node was duplicated
     6) Close Script Canvas window or else test hangs on save confirmation modal

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
        self.sc_graph_view = None
        self.sc_graph = None

    def initialize_sc_graph_qt_objects(self):
        self.sc_graph_view = self.sc_editor.findChild(QtWidgets.QFrame, "graphicsViewFrame")
        self.sc_graph = self.sc_graph_view.findChild(QtWidgets.QWidget, "")

    # Function for entering and executing a string command in the Command Line Tool (SC > Tools > Command Line)
    def run_command_line_input(self, sc_editor, sc_editor_main_window, command_str):
        command_line_action = pyside_utils.find_child_by_pattern(
            sc_editor_main_window, {"objectName": "action_ViewCommandLine", "type": QtWidgets.QAction}
        )
        command_line_action.trigger()
        command_line_textbox = sc_editor.findChild(QtWidgets.QLineEdit, "commandText")
        QtTest.QTest.keyClicks(command_line_textbox, command_str)
        QtTest.QTest.keyClick(command_line_textbox, Qt.Key_Enter, Qt.NoModifier)


    @pyside_utils.wrap_async
    async def run_test(self):

        # Preconditions
        general.idle_enable(True)

        # 1) Open Script Canvas window (Tools > Script Canvas) and initialize Qt objects
        general.open_pane(SCRIPT_CANVAS_UI)
        helper.wait_for_condition(lambda: general.is_pane_visible(SCRIPT_CANVAS_UI), WAIT_TIME_3)
        scripting_tools.initialize_editor_object(self)
        scripting_tools.initialize_sc_editor_objects(self)

        # 2) Open a new graph
        scripting_tools.create_new_sc_graph(self.sc_editor_main_window)
        # Toggle the node inspector if it's not already active
        sc_graph_node_inspector = scripting_tools.get_sc_editor_node_inspector(self.sc_editor)

        # 3) Add a node to the graph by emulating the Command Line tool's workflow
        self.run_command_line_input(self.sc_editor, self.sc_editor_main_window, COMMAND_LINE_ARGS)

        # 4) Duplicate nodes on graph w/ ctrl+a and ctrl+d keystroke
        self.initialize_sc_graph_qt_objects()
        self.sc_editor_main_window.activateWindow()
        QtTest.QTest.keyClick(self.sc_graph, "a", Qt.ControlModifier, WAIT_FRAMES)
        QtTest.QTest.keyClick(self.sc_graph, "d", Qt.ControlModifier, WAIT_FRAMES)

        # 5) Verify the node was duplicated
        # As direct interaction with node is not available the text on the label
        # inside the Node Inspector is validated showing two nodes exist
        node_titles = scripting_tools.get_node_inspector_node_titles(self, sc_graph_node_inspector, self.sc_graph)
        found_title = False
        for title in node_titles:
            if title == EXPECTED_NODE_TITLE_STRING:
                found_title = True
                break

        Report.result(Tests.node_duplicated, found_title)

        # 6) Close Script Canvas window or else test hangs on save confirmation modal
        general.close_pane(SCRIPT_CANVAS_UI)


test = Node_HappyPath_DuplicateNode()
test.run_test()
