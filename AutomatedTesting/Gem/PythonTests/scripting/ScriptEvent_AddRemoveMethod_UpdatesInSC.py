"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
class Tests():
    node_exists = ("Node found in Node Palette", "Node not found in Node Palette")
    method_removed = ("Method removed from scriptevent file", "Method not removed from scriptevent file")

def ScriptEvent_AddRemoveMethod_UpdatesInSC():
    """
    Summary:
     Method can be added/removed to an existing .scriptevents file

    Expected Behavior:
     The Method is correctly added/removed to the asset, and Script Canvas nodes are updated accordingly.

    Test Steps:
     1) Initialize Qt Test objects and open Asset Editor
     2) Create new Script Event file and add two methods
     3) Save the script event file to disk
     4) Open script canvas editor and search the node palette for our new method
     5) Delete one of the methods from script event
     6) Search the node palette for the deleted method. Verify it's gone
     7) close script canvas editor and asset editor

    Note:
     - This test file must be called from the Open 3D Engine Editor command terminal
     - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    """

    # Preconditions
    import os
    import azlmbr.paths as paths
    from editor_python_test_tools.utils import Report
    import azlmbr.legacy.general as general
    from editor_python_test_tools.QtPy.QtPyO3DEEditor import QtPyO3DEEditor
    from consts.asset_editor import (SCRIPT_EVENT_UI, NODE_TEST_METHOD)

    general.idle_enable(True)

    # 1) Get a handle on our O3DE QtPy objects then initialize the Asset Editor object
    qtpy_o3de_editor = QtPyO3DEEditor()
    # Close and reopen Asset Editor to ensure we don't have any existing assets open
    qtpy_o3de_editor.close_asset_editor()
    qtpy_asset_editor = qtpy_o3de_editor.open_asset_editor()

    # 2) Create new Script Event file and add two methods
    qtpy_asset_editor.click_menu_bar_option(SCRIPT_EVENT_UI)

    qtpy_asset_editor.add_method_to_script_event(f"{NODE_TEST_METHOD}_0")
    qtpy_asset_editor.add_method_to_script_event(f"{NODE_TEST_METHOD}_1")

    # 3) Save the script event file to disk
    file_path = os.path.join(paths.projectroot, "TestAssets", "test_file.scriptevents")
    qtpy_asset_editor.save_script_event_file(file_path)

    # 4) Open script canvas editor and search the node palette for our new method
    qtpy_o3de_editor.open_script_canvas()
    node_exists = qtpy_o3de_editor.sc_editor.node_palette.search_for_node(f"{NODE_TEST_METHOD}_0") is not None
    Report.critical_result(Tests.node_exists, node_exists)

    # 5) Delete one of the methods from script event
    qtpy_asset_editor.delete_method_from_script_events()
    qtpy_asset_editor.save_script_event_file(file_path)

    # 6) Search the node palette for the deleted method. Verify it's gone
    node_does_not_exist = qtpy_o3de_editor.sc_editor.node_palette.search_for_node(f"{NODE_TEST_METHOD}_0") is None
    Report.critical_result(Tests.method_removed, node_does_not_exist)

    # 7) close script canvas editor and asset editor
    qtpy_o3de_editor.close_script_canvas()
    qtpy_o3de_editor.close_asset_editor()


if __name__ == "__main__":

    import ImportPathHelper as imports

    imports.init()
    from editor_python_test_tools.utils import Report

    Report.start_test(ScriptEvent_AddRemoveMethod_UpdatesInSC)
