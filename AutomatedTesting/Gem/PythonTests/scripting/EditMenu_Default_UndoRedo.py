"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

VARIABLE_COUNT_BEFORE = 1
VARIABLE_COUNT_AFTER = 0
VARIABLE_NAME = "Test Boolean"

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
     4) Delete the variable and verify it's removed in Variable Manager
     5) Trigger undo action and verify if variable is re-added in Variable Manager
     6) Close SC window

    Note:
     - This test file must be called from the Open 3D Engine Editor command terminal
     - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    # Preconditions
    import azlmbr.legacy.general as general
    from editor_python_test_tools.QtPyO3DEEditor import QtPyO3DEEditor

    general.idle_enable(True)

    # 1) Open Script Canvas window
    qtpy_o3de_editor = QtPyO3DEEditor()
    sc_editor = qtpy_o3de_editor.open_script_canvas()

    # 2) Create Graph
    sc_editor.create_new_script_canvas_graph()

    # 3) Create and verify the new variable exists in variable manager
    variable_manager = sc_editor.variable_manager
    variable_manager.create_new_variable(VARIABLE_NAME, variable_manager.variable_types.Boolean)
    variable_manager.validate_variable_count(VARIABLE_COUNT_BEFORE)

    # 4) Delete the variable and verify it's removed in Variable Manager
    variable_manager.delete_variable(VARIABLE_NAME)
    variable_manager.validate_variable_count(VARIABLE_COUNT_AFTER)
    
    # 5) Trigger undo action and verify if variable is re-added in Variable Manager
    sc_editor.trigger_undo_action()
    variable_manager.validate_variable_count(VARIABLE_COUNT_BEFORE)

    # 6) Close SC window
    qtpy_o3de_editor.close_script_canvas()


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(EditMenu_Default_UndoRedo)
