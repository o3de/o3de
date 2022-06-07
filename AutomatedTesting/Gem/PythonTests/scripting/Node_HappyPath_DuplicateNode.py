"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from PySide2 import QtWidgets, QtTest
from PySide2.QtCore import Qt
from editor_python_test_tools.utils import Report
from editor_python_test_tools.utils import TestHelper as helper
import editor_python_test_tools.pyside_utils as pyside_utils
import azlmbr.legacy.general as general
from scripting_utils.scripting_constants import (SCRIPT_CANVAS_UI, TITLE_STRING, NODE_INSPECTOR_QT, NODE_INSPECTOR_UI,  WAIT_TIME_3)

WAIT_FRAMES = 200
COMMAND_LINE_ARGS = "add_node Print"
EXPECTED_STRING = "Print - Utilities/Debug (2 Selected)"


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
     1) Open Script Canvas window (Tools > Script Canvas)
     2) Open a new graph
     3) Add node to graph
     4) Duplicate node
     5) Verify the node was duplicated6) Verify the node was duplicated

    Note:
     - This test file must be called from the Open 3D Engine Editor command terminal
     - Any passed and failed tests are written to the Editor.log file.
        Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """


    def run_command_line_input(self, sc_editor, sc_editor_main_window, command_str):
        command_line_action = pyside_utils.find_child_by_pattern(
            sc_editor_main_window, {"objectName": "action_ViewCommandLine", "type": QtWidgets.QAction}
        )
        command_line_action.trigger()
        textbox = sc_editor.findChild(QtWidgets.QLineEdit, "commandText")
        QtTest.QTest.keyClicks(textbox, command_str)
        QtTest.QTest.keyClick(textbox, Qt.Key_Enter, Qt.NoModifier)

    def grab_title_text(self, sc_graph_node_inspector, sc_graph):
        scroll_area = sc_graph_node_inspector.findChild(QtWidgets.QScrollArea, "")
        QtTest.QTest.keyClick(sc_graph, "a", Qt.ControlModifier, WAIT_FRAMES)
        background = scroll_area.findChild(QtWidgets.QFrame, "Background")
        title = background.findChild(QtWidgets.QLabel, TITLE_STRING)
        text = title.findChild(QtWidgets.QLabel, TITLE_STRING)
        return text.text()

    @pyside_utils.wrap_async
    async def run_test(self):

        # Preconditions
        general.idle_enable(True)

        # 1) Open Script Canvas window (Tools > Script Canvas)
        general.open_pane(SCRIPT_CANVAS_UI)
        helper.wait_for_condition(lambda: general.is_pane_visible(SCRIPT_CANVAS_UI), WAIT_TIME_3)

        # 2) Open a new graph
        editor_window = pyside_utils.get_editor_main_window()
        sc_editor = editor_window.findChild(QtWidgets.QDockWidget, SCRIPT_CANVAS_UI)
        sc_editor_main_window = sc_editor.findChild(QtWidgets.QMainWindow)
        create_new_graph = pyside_utils.find_child_by_pattern(
            sc_editor_main_window, {"objectName": "action_New_Script", "type": QtWidgets.QAction}
        )
        if sc_editor.findChild(QtWidgets.QDockWidget, NODE_INSPECTOR_QT) is None:
            action = pyside_utils.find_child_by_pattern(sc_editor, {"text": NODE_INSPECTOR_UI, "type": QtWidgets.QAction})
            action.trigger()
        sc_graph_node_inspector = sc_editor.findChild(QtWidgets.QDockWidget, NODE_INSPECTOR_QT)
        create_new_graph.trigger()

        # 3) Add node
        self.run_command_line_input(sc_editor, sc_editor_main_window, COMMAND_LINE_ARGS)

        # 4) Duplicate node w/ a ctrl+
        sc_graph_view = sc_editor.findChild(QtWidgets.QFrame, "graphicsViewFrame")
        sc_graph = sc_graph_view.findChild(QtWidgets.QWidget, "")
        sc_editor_main_window.activateWindow()
        QtTest.QTest.keyClick(sc_graph, "a", Qt.ControlModifier, WAIT_FRAMES)
        QtTest.QTest.keyClick(sc_graph, "d", Qt.ControlModifier, WAIT_FRAMES)

        # 5) Verify the node was duplicated
        # As direct interaction with node is not available the text on the label
        # inside the Node Inspector is validated showing two nodes exist
        after_dup = self.grab_title_text(sc_graph_node_inspector, sc_graph)
        Report.result(Tests.node_duplicated, after_dup == EXPECTED_STRING)

        # 6) Close Script Canvas window or else test hangs on save confirmation modal
        general.close_pane(SCRIPT_CANVAS_UI)


test = Node_HappyPath_DuplicateNode()
test.run_test()
