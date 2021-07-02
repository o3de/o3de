"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# fmt: off
class Tests():
    open_pane      = ("Pane opened successfully",         "Failed to open pane")
    resize_pane    = ("Pane window resized successfully", "Failed to resize pane window")
# fmt: on


def Pane_HappyPath_ResizesProperly():
    """
    Summary:
     The Script Canvas window is opened to verify if Script Canvas panes can be resized and scaled

    Expected Behavior:
     The pane is resized and scaled appropriately.

    Test Steps:
     1) Open Script Canvas window (Tools > Script Canvas)
     2) Restore default layout
     3) Make sure pane is opened
     4) Resize pane and verify change
     5) Restore default layout
     6) Close Script Canvas window

    Note:
     - This test file must be called from the Open 3D Engine Editor command terminal
     - Any passed and failed tests are written to the Editor.log file.
        Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    from PySide2 import QtWidgets

    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper
    import editor_python_test_tools.pyside_utils as pyside_utils

    # Open 3D Engine imports
    import azlmbr.legacy.general as general

    PANE_WIDGET = "NodePalette"
    SCALE_INT = 10

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

    # 2) Restore default layout
    editor_window = pyside_utils.get_editor_main_window()
    sc = editor_window.findChild(QtWidgets.QDockWidget, "Script Canvas")
    click_menu_option(sc, "Restore Default Layout")

    # 3) Make sure pane is opened
    editor_window = pyside_utils.get_editor_main_window()
    sc = editor_window.findChild(QtWidgets.QDockWidget, "Script Canvas")
    pane = find_pane(sc, PANE_WIDGET)
    if not pane.isVisible():
        click_menu_option(sc, "Node Palette")
        pane = find_pane(sc, PANE_WIDGET)  # New reference

    Report.result(Tests.open_pane, pane.isVisible())

    # 4) Resize pane and verify change
    initial_size = pane.frameSize()
    pane.resize(initial_size.width() + SCALE_INT, initial_size.height() + SCALE_INT)
    new_size = pane.frameSize()
    test_success = (
        abs(initial_size.width() - new_size.width()) == abs(initial_size.height() - new_size.height()) == SCALE_INT
    )
    Report.result(Tests.resize_pane, test_success)

    # 5) Restore default layout
    # Needed this step to restore to default in case of test failure
    click_menu_option(sc, "Restore Default Layout")

    # 6) Close Script Canvas window
    sc.close()


if __name__ == "__main__":
    import ImportPathHelper as imports

    imports.init()
    from editor_python_test_tools.utils import Report

    Report.start_test(Pane_HappyPath_ResizesProperly)
