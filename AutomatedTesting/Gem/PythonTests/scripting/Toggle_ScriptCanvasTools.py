"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

Test case ID: C92569165, C92569167, C92569168, C92569170
Test Case Title: Tools > Node Palette toggles the Node Palette
                 Tools > Node Inspector toggles the Node Inspector
                 Tools > Bookmarks toggles the Bookmarks
                 Tools > Variable Manager toggles the Variable Manager
                 
URLs of the test case: https://testrail.agscollab.com/index.php?/cases/view/92569165
                       https://testrail.agscollab.com/index.php?/cases/view/92569167
                       https://testrail.agscollab.com/index.php?/cases/view/92569168
                       https://testrail.agscollab.com/index.php?/cases/view/92569170
"""


# fmt: off
class Tests():
    node_palette_opened              = ("NodePalette is opened successfully",     "Failed to open NodePalette")
    node_inspector_opened            = ("NodeInspector is opened successfully",   "Failed to open NodeInspector")
    bookmark_opened                  = ("Bookmarks is opened successfully",       "Failed to open Bookmarks")
    variable_manager_opened          = ("VariableManager is opened successfully", "Failed to open VariableManager")
    node_palette_closed_by_start     = ("NodePalette is closed successfully",     "Failed to close NodePalette")
    node_inspector_closed_by_start   = ("NodeInspector is closed successfully",   "Failed to close NodeInspector")
    bookmark_closed_by_start         = ("Bookmarks is closed successfully",       "Failed to close Bookmarks")
    variable_manager_closed_by_start = ("VariableManager is closed successfully", "Failed to close VariableManager")
    node_palette_closed_by_end       = ("NodePalette is closed successfully",     "Failed to close NodePalette")
    node_inspector_closed_by_end     = ("NodeInspector is closed successfully",   "Failed to close NodeInspector")
    bookmark_closed_by_end           = ("Bookmarks is closed successfully",       "Failed to close Bookmarks")
    variable_manager_closed_by_end   = ("VariableManager is closed successfully", "Failed to close VariableManager")
# fmt: on


def Toggle_ScriptCanvasTools():
    """
    Summary:
     Toggle Node Palette, Node Inspector, Bookmarks and Variable Manager in Script Canvas.
     Make sure each pane opens and closes successfully.

    Expected Behavior:
     Each pane opens and closes successfully.

    Test Steps:
     1) Open Script Canvas window (Tools > Script Canvas)
     2) Make sure Node Palette, Node Inspector, Bookmarks and Variable Manager panes are closed in Script Canvas window
     3) Open Node Palette, Node Inspector, Bookmarks and Variable Manager in Script Canvas window
     4) Close Node Palette, Node Inspector, Bookmarks and Variable Manager in Script Canvas window
     5) Restore default layout
     6) Close Script Canvas window

    Note:
     - This test file must be called from the Open 3D Engine Editor command terminal
     - Any passed and failed tests are written to the Editor.log file.
        Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    from utils import Report
    from utils import TestHelper as helper
    import pyside_utils

    # Open 3D Engine imports
    import azlmbr.legacy.general as general

    # Pyside imports
    from PySide2 import QtWidgets

    def click_menu_option(window, option_text):
        action = pyside_utils.find_child_by_pattern(window, {"text": option_text, "type": QtWidgets.QAction})
        action.trigger()

    def find_pane(window, pane_name):
        return window.findChild(QtWidgets.QDockWidget, pane_name)

    def close_tool(window, pane_widget, test_tuple):
        pane = find_pane(window, pane_widget)
        pane.close()
        Report.result(test_tuple, not pane.isVisible())

    def open_tool(window, pane_widget, tool, test_tuple):
        pane = find_pane(window, pane_widget)
        if not pane.isVisible():
            click_menu_option(window, tool)
            pane = find_pane(window, pane_widget)
            Report.result(test_tuple, pane.isVisible())

    # Test starts here
    general.idle_enable(True)

    # 1) Open Script Canvas window (Tools > Script Canvas)
    general.open_pane("Script Canvas")
    helper.wait_for_condition(lambda: general.is_pane_visible("Script Canvas"), 5.0)

    # 2) Make sure Node Palette, Node Inspector, Bookmarks and Variable Manager panes are closed in Script Canvas window
    editor_window = pyside_utils.get_editor_main_window()
    sc = editor_window.findChild(QtWidgets.QDockWidget, "Script Canvas")
    close_tool(sc, "NodePalette", Tests.node_palette_closed_by_start)
    close_tool(sc, "NodeInspector", Tests.node_inspector_closed_by_start)
    close_tool(sc, "BookmarkDockWidget", Tests.bookmark_closed_by_start)
    close_tool(sc, "VariableManager", Tests.variable_manager_closed_by_start)

    # 3) Open Node Palette, Node Inspector, Bookmarks and Variable Manager in Script Canvas window
    open_tool(sc, "NodePalette", "Node Palette", Tests.node_palette_opened)
    open_tool(sc, "NodeInspector", "Node Inspector", Tests.node_inspector_opened)
    open_tool(sc, "BookmarkDockWidget", "Bookmarks", Tests.bookmark_opened)
    open_tool(sc, "VariableManager", "Variable Manager", Tests.variable_manager_opened)

    # 4) Close Node Palette, Node Inspector, Bookmarks and Variable Manager in Script Canvas window
    close_tool(sc, "NodePalette", Tests.node_palette_closed_by_end)
    close_tool(sc, "NodeInspector", Tests.node_inspector_closed_by_end)
    close_tool(sc, "BookmarkDockWidget", Tests.bookmark_closed_by_end)
    close_tool(sc, "VariableManager", Tests.variable_manager_closed_by_end)

    # 5) Restore default layout
    # Need this step to restore to default in case of test failure
    click_menu_option(sc, "Restore Default Layout")

    # 6) Close Script Canvas window
    sc.close()


if __name__ == "__main__":
    import ImportPathHelper as imports

    imports.init()

    from utils import Report

    Report.start_test(Toggle_ScriptCanvasTools)
