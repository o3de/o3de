"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
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
    import editor_python_test_tools.script_canvas_tools_qt as sc_tools_qt
    from consts.general import (WAIT_TIME_3)

    general.idle_enable(True)

    # 1) Open Script Canvas window
    sc_tools_qt.open_script_canvas()

    # 2) Create Graph
    sc_tools_qt.create_new_sc_graph()

    # 3) Create and verify the new variable exists in variable manager
    qtpy_variable_manager = sc_tools_qt.EDITOR_QT_CONTAINER.get_variable_manager()
    variable_types = qtpy_variable_manager.get_basic_variable_types()
    qtpy_variable_manager.create_new_variable(variable_types.Boolean)
    row_count = qtpy_variable_manager.sc_variable_count_matches_expected(VARIABLE_COUNT_BEFORE)
    Report.result(Tests.variable_created, helper.wait_for_condition(
        lambda: row_count, WAIT_TIME_3))

    # 4) Trigger Undo action and verify if variable is removed in Variable Manager
    sc_editor_wrapper = sc_tools_qt.EDITOR_QT_CONTAINER.get_script_canvas_editor()
    sc_editor_wrapper.trigger_undo_action()
    row_count = qtpy_variable_manager.sc_variable_count_matches_expected(VARIABLE_COUNT_AFTER)
    Report.result(Tests.undo_worked, helper.wait_for_condition(
        lambda: row_count, WAIT_TIME_3))

    # 5) Trigger Redo action and verify if variable is re-added in Variable Manager
    sc_editor_wrapper.trigger_redo_action()
    row_count = qtpy_variable_manager.sc_variable_count_matches_expected(VARIABLE_COUNT_BEFORE)
    Report.result(Tests.redo_worked, helper.wait_for_condition(
        lambda: row_count, WAIT_TIME_3))

    # 6) Close SC window
    sc_tools_qt.close_script_canvas()


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(EditMenu_Default_UndoRedo)
