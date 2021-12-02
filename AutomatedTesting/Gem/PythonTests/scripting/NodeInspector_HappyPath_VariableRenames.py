"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# fmt: off
class Tests():
    variable_created        = ("New variable created",                    "New variable is not created")
    node_inspector_rename   = ("Variable is renamed in Node Inspector",   "Variable is not renamed in Node Inspector")
    variable_manager_rename = ("Variable is renamed in Variable Manager", "Variable is not renamed in Variable Manager")
# fmt: on


GENERAL_WAIT = 0.5  # seconds


def NodeInspector_HappyPath_VariableRenames():
    """
    Summary:
     Renaming variables in the Node Inspector, renames the actual variable.

    Expected Behavior:
     The Variable's name is changed in both Node Inspector and Variable Manager.

    Test Steps:
     1) Open Script Canvas window (Tools > Script Canvas)
     2) Get the SC window object
     3) Open Variable Manager if not opened already
     4) Open Node Inspector if not opened already
     5) Create new graph and a new variable in Variable manager
     6) Click on the variable
     7) Update name in Node Inspector and click on ENTER
     8) Verify if the name is updated in Node inspector and Variable manager
     9) Close Script Canvas window

    Note:
     - This test file must be called from the Open 3D Engine Editor command terminal
     - Any passed and failed tests are written to the Editor.log file.
        Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """
    from PySide2 import QtWidgets, QtCore, QtTest
    from PySide2.QtCore import Qt

    import pyside_utils
    from utils import TestHelper as helper

    import azlmbr.legacy.general as general

    TEST_NAME = "test name"

    def open_tool(sc, dock_widget_name, pane_name):
        if sc.findChild(QtWidgets.QDockWidget, dock_widget_name) is None:
            action = pyside_utils.find_child_by_pattern(sc, {"text": pane_name, "type": QtWidgets.QAction})
            action.trigger()
        tool = sc.findChild(QtWidgets.QDockWidget, dock_widget_name)
        return tool

    # 1) Open Script Canvas window
    general.idle_enable(True)
    general.open_pane("Script Canvas")
    helper.wait_for_condition(lambda: general.is_pane_visible("Script Canvas"), 5.0)

    # 2) Get the SC window object
    editor_window = pyside_utils.get_editor_main_window()
    sc = editor_window.findChild(QtWidgets.QDockWidget, "Script Canvas")

    # 3) Open Variable Manager if not opened already
    variable_manager = open_tool(sc, "VariableManager", "Variable Manager")

    # 4) Open Node Inspector if not opened already
    node_inspector = open_tool(sc, "NodeInspector", "Node Inspector")

    # 5) Create new graph and a new variable in Variable manager
    action = pyside_utils.find_child_by_pattern(sc, {"objectName": "action_New_Script", "type": QtWidgets.QAction})
    action.trigger()
    graph_vars = variable_manager.findChild(QtWidgets.QTableView, "graphVariables")
    add_button = variable_manager.findChild(QtWidgets.QPushButton, "addButton")
    add_button.click()
    # Select variable type
    table_view = variable_manager.findChild(QtWidgets.QTableView, "variablePalette")
    model_index = pyside_utils.find_child_by_pattern(table_view, "Boolean")
    # Click on it to create variable
    pyside_utils.item_view_index_mouse_click(table_view, model_index)
    result = graph_vars.model().rowCount(QtCore.QModelIndex()) == 1
    var_mi = pyside_utils.find_child_by_pattern(graph_vars, "Variable 1")
    result = result and (var_mi is not None)
    Report.critical_result(Tests.variable_created, result)

    # 6) Click on the variable
    pyside_utils.item_view_index_mouse_click(graph_vars, var_mi)

    # 7) Update name in Node Inspector and click on ENTER
    helper.wait_for_condition(
        lambda: node_inspector.findChild(QtWidgets.QWidget, "ContainerForRows") is not None, GENERAL_WAIT
    )
    row_container = node_inspector.findChild(QtWidgets.QWidget, "ContainerForRows")
    name_frame = row_container.findChild(QtWidgets.QWidget, "Name")
    name_line_edit = name_frame.findChild(QtWidgets.QLineEdit)
    name_line_edit.setText(TEST_NAME)
    QtTest.QTest.keyClick(name_line_edit, Qt.Key_Return, Qt.NoModifier)

    # 8) Verify if the name is updated in Node inspector and Variable manager
    helper.wait_for_condition(lambda: var_mi.data(Qt.DisplayRole) == TEST_NAME, GENERAL_WAIT)
    Report.critical_result(Tests.node_inspector_rename, name_line_edit.text() == TEST_NAME)
    Report.critical_result(Tests.variable_manager_rename, var_mi.data(Qt.DisplayRole) == TEST_NAME)

    # 9) Close Script Canvas window
    general.close_pane("Script Canvas")


if __name__ == "__main__":
    import ImportPathHelper as imports

    imports.init()
    from utils import Report

    Report.start_test(NodeInspector_HappyPath_VariableRenames)
