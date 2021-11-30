"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


def VariableManager_Default_CreateDeleteVars():
    """
    Summary:
     Creating and deleting each type of variable in the Variable Manager pane

    Expected Behavior:
     Each variable type can be created and deleted in variable manager.

    Test Steps:
     1) Open Script Canvas window (Tools > Script Canvas)
     2) Get the SC window object
     3) Open Variable Manager if not opened already
     4) Create Graph
     5) Create variable of each type and verify if it is created
     6) Delete each type of variable and verify if it is deleted
     7) Close SC window

    Note:
     - This test file must be called from the Open 3D Engine Editor command terminal
     - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    from PySide2 import QtWidgets, QtCore, QtTest
    from PySide2.QtCore import Qt

    from utils import TestHelper as helper
    import pyside_utils

    import azlmbr.legacy.general as general

    def generate_test_tuple(var_type, action):
        return (f"{var_type} variable is {action}d", f"{var_type} variable is not {action}d")

    # 1) Open Script Canvas window
    general.idle_enable(True)
    general.open_pane("Script Canvas")
    helper.wait_for_condition(lambda: general.is_pane_visible("Script Canvas"), 10.0)

    # 2) Get the SC window object
    editor_window = pyside_utils.get_editor_main_window()
    sc = editor_window.findChild(QtWidgets.QDockWidget, "Script Canvas")

    # 3) Open Variable Manager if not opened already
    if sc.findChild(QtWidgets.QDockWidget, "VariableManager") is None:
        action = pyside_utils.find_child_by_pattern(sc, {"text": "Variable Manager", "type": QtWidgets.QAction})
        action.trigger()
    variable_manager = sc.findChild(QtWidgets.QDockWidget, "VariableManager")

    # 4) Create Graph
    action = pyside_utils.find_child_by_pattern(sc, {"objectName": "action_New_Script", "type": QtWidgets.QAction})
    action.trigger()

    graph_vars = variable_manager.findChild(QtWidgets.QTableView, "graphVariables")

    var_types = ["Boolean", "Color", "EntityID", "Number", "String", "Transform", "Vector2", "Vector3", "Vector4"]

    # 5) Create variable of each type and verify if it is created
    for index, var_type in enumerate(var_types):
        # Create new variable
        add_button = variable_manager.findChild(QtWidgets.QPushButton, "addButton")
        add_button.click()  # Click on Create Variable button
        # Select variable type
        table_view = variable_manager.findChild(QtWidgets.QTableView, "variablePalette")
        model_index = pyside_utils.find_child_by_pattern(table_view, var_type)
        # Click on it to create variable
        pyside_utils.item_view_index_mouse_click(table_view, model_index)
        # Verify if the variable is created
        # NOTE: To check if variable of a type is created, we are checking 1) rowcount
        # 2) If we have row with variable "Variable <index>"
        # 3) Type of variable, which is next column of the variable name
        result = graph_vars.model().rowCount(QtCore.QModelIndex()) == (
            index + 1
        )  # since we added 1 variable, rowcount will increase by 1
        var_mi = pyside_utils.find_child_by_pattern(graph_vars, f"Variable {index+1}")
        result = result and (var_mi is not None) and (var_mi.siblingAtColumn(1).data(Qt.DisplayRole) == var_type)
        Report.result(generate_test_tuple(var_type, "create"), result)

    # 6) Delete each type of variable and verify if it is deleted
    for index, var_type in enumerate(var_types):
        # Delete variable and verify if its deleted
        # NOTE: To check if variable of a type is deleted, we are checking rowcount
        var_mi = pyside_utils.find_child_by_pattern(graph_vars, f"Variable {index+1}")
        pyside_utils.item_view_index_mouse_click(graph_vars, var_mi)
        QtTest.QTest.keyClick(graph_vars, Qt.Key_Delete, Qt.NoModifier)
        # since variable is deleted, rowcount will decrease by 1
        result = graph_vars.model().rowCount(QtCore.QModelIndex()) == (len(var_types) - (index + 1))
        Report.result(generate_test_tuple(var_type, "delete"), result)

    # 7) Close SC window
    general.close_pane("Script Canvas")


if __name__ == "__main__":
    import ImportPathHelper as imports

    imports.init()
    from utils import Report

    Report.start_test(VariableManager_Default_CreateDeleteVars)
