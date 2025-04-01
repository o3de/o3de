"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# fmt: off
class Tests():
    set_search_string     = ("Search string is set",              "Search string is not set")
# fmt: on

def NodePalette_HappyPath_ClearSelection():
    """
    Summary:
     Clicking the X button on the Search Box clears the currently entered string

    Expected Behavior:
     After entering a string value into the Node Palette's search box and click on
     the X button, the search box should be cleared

    Test Steps:
     1) Initialize our O3DE editor object
     2) Open and initialize Script Canvas window (Tools > Script Canvas)
     3) Search for a node (this test uses a node from the Math category)
     4) Verify the node was found
     5) Clear search strin

    Note:
     - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    #Preconditions
    from editor_python_test_tools.utils import Report
    import azlmbr.legacy.general as general
    from editor_python_test_tools.QtPy.QtPyO3DEEditor import QtPyO3DEEditor
    from consts.scripting import (NODE_STRING_TO_NUMBER)

    general.idle_enable(True)

    # 1) Initialize our O3DE editor object
    qtpy_o3de_editor = QtPyO3DEEditor()

    # 2) Open and initialize Script Canvas test objects (Tools > Script Canvas)
    qtpy_o3de_editor.open_script_canvas()
    qtpy_node_palette = qtpy_o3de_editor.sc_editor.node_palette

    # 3) Search for a node (this test uses a node from the Math category)
    node_palette_search_results = qtpy_node_palette.search_for_node(NODE_STRING_TO_NUMBER)

    # 4) Verify the node was found
    Report.result(Tests.set_search_string, node_palette_search_results is not None)

    # 5) Clear search string
    qtpy_node_palette.click_clear_search_button()


if __name__ == "__main__":
    import ImportPathHelper as imports

    imports.init()
    from utils import Report

    Report.start_test(NodePalette_HappyPath_ClearSelection)
