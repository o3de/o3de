"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

def ScriptEvent_AddRemoveParameter_ActionsSuccessful():
    """
    Summary:
     Parameter can be removed from a Script Event method

    Expected Behavior:
     clicking the "+" parameter button adds a parameter to a script event.
     Clicking the trash can button removes the parameter

    Test Steps:
     1) Open Asset Editor
     2) Initialize the editor and asset editor qt objects
     3) Create new Script Event Asset
     4) Add Parameter to Event
     5) Remove Parameter from Event

    Note:
     - Any passed and failed tests are written to the Editor.log file.
        Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    # Preconditions
    import os
    from PySide2 import QtWidgets
    from editor_python_test_tools.utils import Report
    import pyside_utils
    import scripting_utils.scripting_tools as tools
    import azlmbr.legacy.general as general
    from editor_python_test_tools.QtPy.QtPyO3DEEditor import QtPyO3DEEditor
    from scripting_utils.scripting_constants import (ASSET_EDITOR_UI, SCRIPT_EVENT_UI)

    general.idle_enable(True)
    general.close_pane(ASSET_EDITOR_UI)

    # 1) Get a handle on our O3DE QtPy objects then initialize the Asset Editor object
    qtpy_o3de_editor = QtPyO3DEEditor()
    # Close and reopen Asset Editor to ensure we don't have any existing assets open
    qtpy_o3de_editor.close_asset_editor()
    qtpy_asset_editor = qtpy_o3de_editor.open_asset_editor()

    # 2) Create new Script Event Asset
    qtpy_asset_editor.click_menu_bar_option(SCRIPT_EVENT_UI)

    # 3) Add a method to the script event
    qtpy_asset_editor.add_method_to_script_event("test_method")

    # 4) Add Parameter to Event
    qtpy_asset_editor.add_parameter_to_method("test_parameter_0")

    # 5) Remove Parameter from Event
    qtpy_asset_editor.delete_parameter_from_method(0)


if __name__ == "__main__":

    import ImportPathHelper as imports

    imports.init()
    from editor_python_test_tools.utils import Report

    Report.start_test(ScriptEvent_AddRemoveParameter_ActionsSuccessful)
    