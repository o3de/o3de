"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""


# fmt: off
class Tests():
    open_sc_window  = ("Script Canvas window is opened",   "Failed to open Script Canvas window")
    node_added      = ("Successfully added node to graph", "Failed to add node to graph")
    node_duplicated = ("Successfully duplicated node",     "Failed to duplicate the node")
# fmt: on


def Node_HappyPath_DuplicateNode():
    """
    Summary:
     Duplicating node in graph

    Expected Behavior:
     Upon selecting a node and pressing Ctrl+D, the node will be duplicated

    Test Steps:
     1) Open Script Canvas window (Tools > Script Canvas)
     2) Open a new graph
     3) Add node to graph
     4) Select node in graph to verify existence
     5) Mock Ctrl+D to duplicate node
     6) Verify the node was duplicated

    Note:
     - This test file must be called from the Open 3D Engine Editor command terminal
     - Any passed and failed tests are written to the Editor.log file.
        Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """
    from PySide2 import QtWidgets, QtTest
    from PySide2.QtCore import Qt

    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper
    import editor_python_test_tools.pyside_utils as pyside_utils

    import azlmbr.legacy.general as general

    WAIT_FRAMES = 200

    NODE_NAME = "Print"
    NODE_CATEGORY = "Debug"
    EXPECTED_STRING = f"{NODE_NAME} - {NODE_CATEGORY} (2 Selected)"

    def command_line_input(command_str):
        cmd_action = pyside_utils.find_child_by_pattern(
            sc_main, {"objectName": "action_ViewCommandLine", "type": QtWidgets.QAction}
        )
        cmd_action.trigger()
        textbox = sc.findChild(QtWidgets.QLineEdit, "commandText")
        QtTest.QTest.keyClicks(textbox, command_str)
        QtTest.QTest.keyClick(textbox, Qt.Key_Enter, Qt.NoModifier)

    def grab_title_text():
        scroll_area = node_inspector.findChild(QtWidgets.QScrollArea, "")
        QtTest.QTest.keyClick(graph, "a", Qt.ControlModifier, WAIT_FRAMES)
        general.idle_wait(1.0)
        background = scroll_area.findChild(QtWidgets.QFrame, "Background")
        title = background.findChild(QtWidgets.QLabel, "Title")
        text = title.findChild(QtWidgets.QLabel, "Title")
        print(text.text())
        return text.text()

    # 1) Open Script Canvas window (Tools > Script Canvas)
    general.idle_enable(True)
    general.open_pane("Script Canvas")
    helper.wait_for_condition(lambda: general.is_pane_visible("Script Canvas"), 5.0)

    # # 2) Open a new graph
    editor_window = pyside_utils.get_editor_main_window()
    sc = editor_window.findChild(QtWidgets.QDockWidget, "Script Canvas")
    sc_main = sc.findChild(QtWidgets.QMainWindow)
    create_new_graph = pyside_utils.find_child_by_pattern(
        sc_main, {"objectName": "action_New_Script", "type": QtWidgets.QAction}
    )
    node_inspector = sc.findChild(QtWidgets.QDockWidget, "NodeInspector")
    create_new_graph.trigger()

    # 3) Add node
    command_line_input("add_node Print")

    # 4) Select node in graph to verify existence
    graph_view = sc.findChild(QtWidgets.QFrame, "graphicsViewFrame")
    graph = graph_view.findChild(QtWidgets.QWidget, "")

    # 5) Duplicate node
    sc_main.activateWindow()
    QtTest.QTest.keyClick(graph, "a", Qt.ControlModifier, WAIT_FRAMES)
    QtTest.QTest.keyClick(graph, "d", Qt.ControlModifier, WAIT_FRAMES)

    # 6) Verify the node was duplicated
    after_dup = grab_title_text()
    Report.result(Tests.node_duplicated, after_dup == EXPECTED_STRING)


if __name__ == "__main__":
    import ImportPathHelper as imports

    imports.init()
    from editor_python_test_tools.utils import Report

    Report.start_test(Node_HappyPath_DuplicateNode)
