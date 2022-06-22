"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
from PySide2 import QtWidgets
import PySide2.QtCore as QtCore
from editor_python_test_tools.utils import TestHelper as helper
from editor_python_test_tools.utils import Report
import azlmbr.legacy.general as general
import editor_python_test_tools.pyside_utils as pyside_utils
import scripting_utils.scripting_tools as scripting_tools
from scripting_utils.scripting_constants import (WAIT_TIME_3, SCRIPT_CANVAS_UI, VARIABLE_TYPES, GRAPH_VARIABLES_QT)

# fmt: off
class Tests():
    variable_created = ("New variable created", "New variable not created")
    undo_worked      = ("Undo action working",  "Undo action did not work")
    redo_worked      = ("Redo action working",  "Redo action did not work")
# fmt: on


class EditMenu_Default_UndoRedo:
    """
    Summary:
     Edit > Undo undoes the last action
     Edit > Redo redoes the last undone action
     We create a new variable in variable manager, undo and verify if variable is removed,
     redo it and verify if the variable is created again.

    Expected Behavior:
     The last action is undone upon selecting Undo.
     The last undone action is redone upon selecting Redo.

    Test Steps:
     1) Open Script Canvas window (Tools > Script Canvas)
     2) Get the SC window object
     3) Open Variable Manager if not opened already
     4) Create Graph
     5) Create new variable
     6) Verify if the variable is created initially
     7) Trigger Undo action and verify if variable is removed in Variable Manager
     8) Trigger Redo action and verify if variable is re-added in Variable Manager
     9) Close SC window

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
        self.variable_manager = None

    def get_graph_variables_row_count(self, graph_variables):
        return graph_variables.model().rowCount(QtCore.QModelIndex())


    @pyside_utils.wrap_async
    async def run_test(self):

        # Preconditions
        general.idle_enable(True)
        scripting_tools.initialize_editor_object(self)

        # 1) Open Script Canvas window
        general.open_pane(SCRIPT_CANVAS_UI)
        helper.wait_for_condition(lambda: general.is_pane_visible(SCRIPT_CANVAS_UI), WAIT_TIME_3)

        # 2) Get the SC window object
        scripting_tools.initialize_sc_editor_objects(self)

        # 3) Open Variable Manager if not opened already
        scripting_tools.initialize_variable_manager_object(self)

        # 4) Create Graph
        scripting_tools.create_new_sc_graph(self.sc_editor_main_window)

        # 5) Create new variable
        scripting_tools.create_new_variable(self, VARIABLE_TYPES[0])

        # 6) Verify if the variable is created initially
        graph_variables = self.variable_manager.findChild(QtWidgets.QTableView, GRAPH_VARIABLES_QT)
        # since we added 1 variable, rowcount=1
        Report.result(Tests.variable_created, helper.wait_for_condition(
            lambda: self.get_graph_variables_row_count(graph_variables) == 1, WAIT_TIME_3))

        # 7) Trigger Undo action and verify if variable is removed in Variable Manager
        undo_redo_action = self.sc_editor.findChild(QtWidgets.QAction, "action_Undo")
        undo_redo_action.trigger()
        # since we triggered undo, rowcount=0
        Report.result(Tests.undo_worked, helper.wait_for_condition(
            lambda: self.get_graph_variables_row_count(graph_variables) == 0, WAIT_TIME_3))

        # 8) Trigger Redo action and verify if variable is re-added in Variable Manager
        undo_redo_action = self.sc_editor.findChild(QtWidgets.QAction, "action_Redo")
        undo_redo_action.trigger()
        # since action is redone 1 variable is re-added, rowcount=1
        Report.result(Tests.redo_worked, helper.wait_for_condition(
            lambda: self.get_graph_variables_row_count(graph_variables) == 1, WAIT_TIME_3))

        # 9) Close SC window
        general.close_pane(SCRIPT_CANVAS_UI)


test = EditMenu_Default_UndoRedo()
test.run_test()
