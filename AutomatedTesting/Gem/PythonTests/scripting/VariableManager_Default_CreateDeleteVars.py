"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from PySide2 import QtWidgets, QtTest, QtCore
from editor_python_test_tools.utils import TestHelper as helper
from editor_python_test_tools.utils import Report
from scripting_utils.scripting_constants import (WAIT_TIME_3, SCRIPT_CANVAS_UI, VARIABLE_MANAGER_UI,
                                                 VARIABLE_MANAGER_QT, VARIABLE_PALETTE_QT, GRAPH_VARIABLES_QT,
                                                 ADD_BUTTON_QT, VARIABLE_TYPES)
import azlmbr.legacy.general as general
import pyside_utils


class VariableManager_Default_CreateDeleteVars:
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
    @staticmethod
    def generate_variable_test_output(var_type, action):
        return f"{var_type} variable is {action}d"

    @pyside_utils.wrap_async
    async def run_test(self):

        # Preconditions
        general.idle_enable(True)

        # 1) Open Script Canvas window
        general.open_pane(SCRIPT_CANVAS_UI)
        helper.wait_for_condition(lambda: general.is_pane_visible(SCRIPT_CANVAS_UI), WAIT_TIME_3)

        # 2) Get the SC window object
        editor_window = pyside_utils.get_editor_main_window()
        sc = editor_window.findChild(QtWidgets.QDockWidget, SCRIPT_CANVAS_UI)

        # 3) Open Variable Manager if not opened already
        if sc.findChild(QtWidgets.QDockWidget, VARIABLE_MANAGER_QT) is None:
            action = pyside_utils.find_child_by_pattern(sc, {"text": VARIABLE_MANAGER_UI, "type": QtWidgets.QAction})
            action.trigger()
        variable_manager = sc.findChild(QtWidgets.QDockWidget, VARIABLE_MANAGER_QT)

        # 4) Create Graph
        action = pyside_utils.find_child_by_pattern(sc, {"objectName": "action_New_Script", "type": QtWidgets.QAction})
        action.trigger()

        # Keep a handle of the created variables for verification
        graph_vars = variable_manager.findChild(QtWidgets.QTableView, GRAPH_VARIABLES_QT)

        # 5) Create variable_type of each type and verify if it is created
        for index, variable_type in enumerate(VARIABLE_TYPES):
            # Create new variable_type button
            add_button = variable_manager.findChild(QtWidgets.QPushButton, ADD_BUTTON_QT)
            # Click on Create Variable button and wait for it to render
            add_button.click()
            helper.wait_for_condition((
                lambda: variable_manager.findChild(QtWidgets.QTableView, VARIABLE_PALETTE_QT) is not None), WAIT_TIME_3)

            # Find the variable_type option in the rendered table and click on it
            table_view = variable_manager.findChild(QtWidgets.QTableView, VARIABLE_PALETTE_QT)
            model_index = pyside_utils.find_child_by_pattern(table_view, variable_type)
            pyside_utils.item_view_index_mouse_click(table_view, model_index)

            # Verify number of created variables in list and new variable's type
            row_count_increased = helper.wait_for_condition(lambda: graph_vars.model().rowCount(QtCore.QModelIndex()) == (
                index + 1), WAIT_TIME_3)
            new_var_entry = pyside_utils.find_child_by_pattern(graph_vars, f"Variable {index+1}")
            new_var_type = new_var_entry.siblingAtColumn(1).data(QtCore.Qt.DisplayRole)

            result = row_count_increased and (new_var_entry is not None) and (new_var_type == variable_type)

            test_output = self.generate_variable_test_output(variable_type, "create")
            Report.info(f"{test_output}: {result}")

        # 6) Delete each type of variable_type and verify if it is deleted
        for index, variable_type in enumerate(VARIABLE_TYPES):
            var_entry = pyside_utils.find_child_by_pattern(graph_vars, f"Variable {index+1}")
            pyside_utils.item_view_index_mouse_click(graph_vars, var_entry)
            QtTest.QTest.keyClick(graph_vars, QtCore.Qt.Key_Delete, QtCore.Qt.NoModifier)

            result = graph_vars.model().rowCount(QtCore.QModelIndex()) == (len(VARIABLE_TYPES) - (index + 1))

            test_output = self.generate_variable_test_output(variable_type, "delete")
            Report.info(f"{test_output}: {result}")

        # 7) Close SC window
        general.close_pane(SCRIPT_CANVAS_UI)


test = VariableManager_Default_CreateDeleteVars()
test.run_test()
