"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
"""
from PySide2 import QtWidgets
import PySide2.QtCore as QtCore
from editor_python_test_tools.utils import TestHelper as helper
from editor_python_test_tools.utils import Report
import azlmbr.legacy.general as general
import pyside_utils
import scripting_utils.scripting_tools as scripting_tools
from scripting_utils.scripting_constants import (WAIT_TIME_3, SCRIPT_CANVAS_UI, VARIABLE_TYPES, GRAPH_VARIABLES_QT)
"""
# fmt: off
class Tests():
    variable_created = ("New variable created", "New variable not created")
    undo_worked      = ("Undo action working",  "Undo action did not work")
    redo_worked      = ("Redo action working",  "Redo action did not work")
# fmt: on

VARIABLE_COUNT_BEFORE = 1
VARIABLE_COUNT_AFTER = 0

def EditMenu_Default_UndoRedo():
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
     1) Open Script Canvas window
     2) Create Graph
     3) Create and verify the new variable exists in variable manager
     4) Trigger Undo action and verify if variable is removed in Variable Manager
     5) Trigger Redo action and verify if variable is re-added in Variable Manager
     6) Close SC window

    Note:
     - This test file must be called from the Open 3D Engine Editor command terminal
     - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """


    # Preconditions
    from editor_python_test_tools.utils import TestHelper as helper
    from editor_python_test_tools.utils import Report
    import azlmbr.legacy.general as general
    import editor_python_test_tools.script_canvas_tools_qt as SC_tools_qt
    from scripting_utils.scripting_constants import (WAIT_TIME_3, SCRIPT_CANVAS_UI)

    general.idle_enable(True)
    variable_types = SC_tools_qt.get_variable_types()

    # 1) Open Script Canvas window
    SC_tools_qt.open_script_canvas()

    # 2) Create Graph
    SC_tools_qt.create_new_sc_graph()

    # 3) Create and verify the new variable exists in variable manager
    SC_tools_qt.create_new_SC_variable(variable_types.Boolean)
    row_count = SC_tools_qt.verify_SC_variable_count(VARIABLE_COUNT_BEFORE)
    Report.result(Tests.variable_created, helper.wait_for_condition(
        lambda: row_count, WAIT_TIME_3))

    # 4) Trigger Undo action and verify if variable is removed in Variable Manager
    SC_tools_qt.script_canvas_undo_action()
    row_count = SC_tools_qt.verify_SC_variable_count(VARIABLE_COUNT_AFTER)
    Report.result(Tests.undo_worked, helper.wait_for_condition(
        lambda: row_count, WAIT_TIME_3))

    # 5) Trigger Redo action and verify if variable is re-added in Variable Manager
    SC_tools_qt.script_canvas_redo_action()
    row_count = SC_tools_qt.verify_SC_variable_count(VARIABLE_COUNT_BEFORE)
    Report.result(Tests.redo_worked, helper.wait_for_condition(
        lambda: row_count, WAIT_TIME_3))

    # 6) Close SC window
    SC_tools_qt.close_script_canvas()


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(EditMenu_Default_UndoRedo)
