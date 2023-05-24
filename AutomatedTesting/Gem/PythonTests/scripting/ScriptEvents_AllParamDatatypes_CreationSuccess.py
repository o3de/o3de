"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

N_VAR_TYPES = 10  # Top 10 variable types

def TestScriptEvents_AllParamDatatypes_CreationSuccess():
    """
       Summary:
        Parameters of all types can be created.

       Expected Behavior:
        The Method handles the large number of Parameters gracefully.
        Parameters of all data types can be successfully created.
        Updated ScriptEvent toast appears in Script Canvas.

       Test Steps:
        1) Get a handle on our O3DE QtPy objects then initialize the Asset Editor object
        2) Create new Script Event Asset
        3) Add a method to the script event
        4) Add new parameters of each type
        5) Close Asset Editor

       Note:
        - Any passed and failed tests are written to the Editor.log file.
               Parsing the file or running a log_monitor are required to observe the test results.

       :return: None
       """
    import azlmbr.legacy.general as general
    from editor_python_test_tools.QtPy.QtPyO3DEEditor import QtPyO3DEEditor
    from scripting_utils.scripting_constants import (SCRIPT_CANVAS_UI, ASSET_EDITOR_UI, SCRIPT_EVENT_UI)

    # Preconditions
    general.idle_enable(True)

    # 1) Get a handle on our O3DE QtPy objects then initialize the Asset Editor object
    qtpy_o3de_editor = QtPyO3DEEditor()
    # Close and reopen Asset Editor to ensure we don't have any existing assets open
    qtpy_o3de_editor.close_asset_editor()
    qtpy_asset_editor = qtpy_o3de_editor.open_asset_editor()

    # 2) Create new Script Event Asset
    qtpy_asset_editor.click_menu_bar_option(SCRIPT_EVENT_UI)

    # 3) Add a method to the script event
    qtpy_asset_editor.add_method_to_script_event("test_method")

    # 4) Add new parameters of each type
    for i in range(0, N_VAR_TYPES):
        qtpy_asset_editor.add_parameter_to_method(f"test_parameter_{i}", i)
        qtpy_asset_editor.change_parameter_type(i, i)

    # 5) Close Asset Editor
    general.close_pane(ASSET_EDITOR_UI)
    general.close_pane(SCRIPT_CANVAS_UI)


if __name__ == "__main__":

    import ImportPathHelper as imports

    imports.init()
    from editor_python_test_tools.utils import Report

    Report.start_test(TestScriptEvents_AllParamDatatypes_CreationSuccess)
    