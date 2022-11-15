"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

def VariableManager_UnpinVariableType_Works():
    """
    Summary:
     Unpin variable types in create variable menu.
    Expected Behavior:
     The variable unpinned in create variable menu remains unpinned after reopening create variable menu.

    Test Steps:
     1) Open Script Canvas window
     2) Get the SC window and variable manager
     3) Create a new sc graph
     4) Unpin Boolean by clicking the "Pin" icon on its left side and verify it's toggled
     5) Restore default layout and close SC window

    Note:
       - Any passed and failed tests are written to the Editor.log file.
        Parsing the file or running a log_monitor are required to observe the test results.
    :return: None
    """

    # Preconditions
    from editor_python_test_tools.QtPyO3DEEditor import QtPyO3DEEditor
    import azlmbr.legacy.general as general
    from consts.scripting import (SCRIPT_CANVAS_UI, RESTORE_DEFAULT_LAYOUT)

    general.idle_enable(True)

    # 1) Open Script Canvas window
    qtpy_o3de_editor = QtPyO3DEEditor()

    # 2) Get the SC window and variable manager
    sc_editor = qtpy_o3de_editor.open_script_canvas()
    variable_manager = sc_editor.variable_manager

    # 3) Create a new sc graph
    sc_editor.create_new_script_canvas_graph()

    # 4) Unpin Boolean by clicking the "Pin" icon on its left side and verify it's toggled
    boolean_type = variable_manager.get_basic_variable_types().Boolean
    variable_manager.toggle_pin_variable_button(boolean_type)

    # 5) Restore default layout and close SC window
    sc_editor.click_menu_bar_option(RESTORE_DEFAULT_LAYOUT)
    general.close_pane(SCRIPT_CANVAS_UI)


if __name__ == "__main__":

    import ImportPathHelper as imports

    imports.init()
    from editor_python_test_tools.utils import Report

    Report.start_test(VariableManager_UnpinVariableType_Works)
