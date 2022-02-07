"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# fmt: off
class Tests():
    pane_opened = ("Pane is opened successfully", "Failed to open pane")
    dock_pane   = ("Pane is docked successfully", "Failed to dock Pane into one or more allowed area")
# fmt: on


def Pane_HappyPath_DocksProperly():
    """
    Summary:
     The Script Canvas window is opened to verify if Script canvas panes can be docked into every
     possible area of Script Canvas main window. (top, bottom, right and left sides of the window)

    Expected Behavior:
     The pane docks successfully.

    Test Steps:
     1) Open Script Canvas window (Tools > Script Canvas)
     2) Make sure Node Palette pane is opened
     3) Dock the Node Palette pane into every possible area of main window
     4) Restore default layout
     5) Close Script Canvas window

    Note:
     - This test file must be called from the Open 3D Engine Editor command terminal
     - Any passed and failed tests are written to the Editor.log file.
        Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper
    import editor_python_test_tools.pyside_utils as pyside_utils

    # Open 3D Engine imports
    import azlmbr.legacy.general as general

    # Pyside imports
    from PySide2 import QtCore, QtWidgets
    from PySide2.QtCore import Qt

    PANE_WIDGET = "NodePalette"  # Chosen most commonly used pane
    DOCKAREAS = [Qt.TopDockWidgetArea, Qt.BottomDockWidgetArea, Qt.RightDockWidgetArea, Qt.LeftDockWidgetArea]
    DOCKED = True

    def click_menu_option(window, option_text):
        action = pyside_utils.find_child_by_pattern(window, {"text": option_text, "type": QtWidgets.QAction})
        action.trigger()

    def find_pane(window, pane_name):
        return window.findChild(QtWidgets.QDockWidget, pane_name)

    # Test starts here
    general.idle_enable(True)

    # 1) Open Script Canvas window (Tools > Script Canvas)
    general.open_pane("Script Canvas")
    helper.wait_for_condition(lambda: general.is_pane_visible("Script Canvas"), 5.0)

    # 2) Make sure Node Palette pane is opened
    editor_window = pyside_utils.get_editor_main_window()
    sc = editor_window.findChild(QtWidgets.QDockWidget, "Script Canvas")
    sc_main = sc.findChild(QtWidgets.QMainWindow)
    pane = find_pane(sc_main, PANE_WIDGET)
    if not pane.isVisible():
        click_menu_option(sc, "Node Palette")
        pane = find_pane(sc_main, PANE_WIDGET)  # New reference

    Report.result(Tests.pane_opened, pane.isVisible())

    # 3) Dock the Node Palette pane into every possible area of main window
    for area in DOCKAREAS:
        sc_main.addDockWidget(area, find_pane(sc_main, PANE_WIDGET), QtCore.Qt.Vertical)
        if not (sc_main.dockWidgetArea(find_pane(sc_main, PANE_WIDGET)) == area):
            Report.info(f"Failed to dock into {str(area)}")
        DOCKED = DOCKED and (sc_main.dockWidgetArea(find_pane(sc_main, PANE_WIDGET)) == area)

    Report.result(Tests.dock_pane, DOCKED)

    # 4) Restore default layout
    # Need this step to restore to default in case of test failure
    click_menu_option(sc, "Restore Default Layout")

    # 5) Close Script Canvas window
    sc.close()


if __name__ == "__main__":
    import ImportPathHelper as imports

    imports.init()
    from editor_python_test_tools.utils import Report

    Report.start_test(Pane_HappyPath_DocksProperly)
