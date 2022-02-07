"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# fmt: off
class Tests():
    undock_pane     = ("Pane is undocked successfully",  "Failed to undock pane")
    close_sc_window = ("Script Canvas window is closed", "Failed to close Script Canvas window")
    pane_closed     = ("Pane is closed successfully",    "Failed to close the pane")
# fmt: on


def Pane_Undocked_ClosesSuccessfully():
    """
    Summary:
     The Script Canvas window is opened with one of the pane undocked.
     Verify if undocked pane closes upon closing Script canvas window.

    Expected Behavior:
     The undocked pane closes when Script Canvas window closed.

    Test Steps:
     1) Open Script Canvas window (Tools > Script Canvas)
     2) Undock Node Palette pane
     3) Connect to Pane visibility signal emitter to verify pane closed
     4) Close Script Canvas window
     5) Restore default layout

    Note:
     - This test file must be called from the Open 3D Engine Editor command terminal
     - Any passed and failed tests are written to the Editor.log file.
        Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    from PySide2 import QtWidgets

    # Helper imports
    from utils import Report
    from utils import TestHelper as helper
    import pyside_utils

    # Open 3D Engine imports
    import azlmbr.legacy.general as general

    TEST_PANE = "NodePalette"  # Chosen most commonly used pane

    def click_menu_option(window, option_text):
        action = pyside_utils.find_child_by_pattern(window, {"text": option_text, "type": QtWidgets.QAction})
        action.trigger()

    def find_pane(window, pane_name):
        return window.findChild(QtWidgets.QDockWidget, pane_name)

    def on_top_level_changed():
        # This function has test condition always True since it gets emitted only when condition satisfied
        Report.result(Tests.undock_pane, True)

    def on_pane_closed():
        # This function has test condition always True since it gets emitted only when condition satisfied
        Report.result(Tests.pane_closed, True)

    # Test starts here
    general.idle_enable(True)

    # 1) Open Script Canvas window (Tools > Script Canvas)
    general.open_pane("Script Canvas")
    helper.wait_for_condition(lambda: general.is_pane_visible("Script Canvas"), 5.0)

    # 2) Undock Node Palette pane
    # Make sure Node Palette pane is opened
    editor_window = pyside_utils.get_editor_main_window()
    sc = editor_window.findChild(QtWidgets.QDockWidget, "Script Canvas")
    pane = find_pane(sc, TEST_PANE)
    if not pane.isVisible():
        click_menu_option(sc, "Node Palette")
        pane = find_pane(sc, TEST_PANE)  # New reference

    # We drag/drop pane over the graph since it doesn't allow docking, so this will undock it
    try:
        graph = find_pane(sc, "GraphCanvasEditorCentralWidget")
        try:
            pane.topLevelChanged.connect(on_top_level_changed)
            pyside_utils.drag_and_drop(pane, graph)
        finally:
            pane.topLevelChanged.disconnect(on_top_level_changed)

        # 3) Connect to Pane visibility signal emitter to verify pane closed
        # No need to disconnect this since pane widget gets deleted when SC window closed
        pane.visibilityChanged.connect(on_pane_closed)

        # 4) Close Script Canvas window
        sc.close()
        is_sc_visible = helper.wait_for_condition(lambda: general.is_pane_visible("Script Canvas"), 2.0)
        Report.result(Tests.close_sc_window, not is_sc_visible)

    finally:
        # 5) Restore default layout
        general.open_pane("Script Canvas")
        helper.wait_for_condition(lambda: general.is_pane_visible("Script Canvas"), 5.0)
        sc = editor_window.findChild(QtWidgets.QDockWidget, "Script Canvas")
        click_menu_option(sc, "Restore Default Layout")
        sc.close()


if __name__ == "__main__":
    import ImportPathHelper as imports

    imports.init()
    from utils import Report

    Report.start_test(Pane_Undocked_ClosesSuccessfully)
