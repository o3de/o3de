"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# fmt: off
class Tests():
    relaunch_sc         = ("Script Canvas window is relaunched",              "Failed to relaunch Script Canvas window")
    test_panes_visible  = ("All the test panes are opened",                   "Failed to open one or more test panes")
    close_pane_1        = ("Test pane 1 is closed",                           "Failed to close test pane 1")
    visibility_retained = ("Test pane retained its visibility on SC restart", "Failed to retain visibility of test pane on SC restart")
    resize_pane_3       = ("Test pane 3 resized successfully",                "Failed to resize Test pane 3")
    size_retained       = ("Test pane retained its size on SC restart",       "Failed to retain size of test pane on SC restart")
    location_changed    = ("Location of test pane 2 changed successfully",    "Failed to change location of test pane 2")
    location_retained   = ("Test pane retained its location on SC restart",   "Failed to retain location of test pane on SC restart")
# fmt: on


def Pane_Default_RetainOnSCRestart():
    """
    Summary:
     The Script Canvas window is opened to verify if Script canvas panes can retain its visibility, size and location
     upon ScriptCanvas restart.

    Expected Behavior:
     The ScriptCanvas pane retain it's visibility, size and location upon ScriptCanvas restart.

    Test Steps:
     1) Open Script Canvas window (Tools > Script Canvas)
     2) Make sure test panes are open and visible
     3) Close test pane 1
     4) Change dock location of test pane 2
     5) Resize test pane 3
     6) Relaunch Script Canvas
     7) Verify if test pane 1 retain its visibility
     8) Verify if location of test pane 2 is retained
     9) Verify if size of test pane 3 is retained
     10) Restore default layout and close SC window

    Note:
     - This test file must be called from the Open 3D Engine Editor command terminal
     - Any passed and failed tests are written to the Editor.log file.
        Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    # Pyside imports
    from PySide2 import QtCore, QtWidgets
    from PySide2.QtCore import Qt

    # Helper imports
    from utils import Report
    from utils import TestHelper as helper
    import pyside_utils

    # Open 3D Engine Imports
    import azlmbr.legacy.general as general

    TEST_PANE_1 = "NodePalette"  # test visibility
    TEST_PANE_2 = "VariableManager"  # test location
    TEST_PANE_3 = "NodeInspector"  # test size
    SCALE_INT = 10  # Random resize scale integer
    DOCKAREA = Qt.TopDockWidgetArea  # Preferred top area since no widget is docked on top

    def click_menu_option(window, option_text):
        action = pyside_utils.find_child_by_pattern(window, {"text": option_text, "type": QtWidgets.QAction})
        action.trigger()

    def find_pane(window, pane_name):
        return window.findChild(QtWidgets.QDockWidget, pane_name)

    # Test starts here
    general.idle_enable(True)

    # 1) Open Script Canvas window (Tools > Script Canvas)
    general.open_pane("Script Canvas")
    helper.wait_for_condition(lambda: general.is_pane_visible("Script Canvas"), 3.0)

    # 2) Make sure test panes are open and visible
    editor_window = pyside_utils.get_editor_main_window()
    sc = editor_window.findChild(QtWidgets.QDockWidget, "Script Canvas")
    click_menu_option(sc, "Restore Default Layout")
    test_pane_1 = sc.findChild(QtWidgets.QDockWidget, TEST_PANE_1)
    test_pane_2 = sc.findChild(QtWidgets.QDockWidget, TEST_PANE_2)
    test_pane_3 = sc.findChild(QtWidgets.QDockWidget, TEST_PANE_3)

    Report.result(
        Tests.test_panes_visible, test_pane_1.isVisible() and test_pane_2.isVisible() and test_pane_3.isVisible()
    )

    # Initiate try block here to restore default in finally block
    try:
        # 3) Close test pane
        test_pane_1.close()
        Report.result(Tests.close_pane_1, not test_pane_1.isVisible())

        # 4) Change dock location of test pane 2
        sc_main = sc.findChild(QtWidgets.QMainWindow)
        sc_main.addDockWidget(DOCKAREA, find_pane(sc_main, TEST_PANE_2), QtCore.Qt.Vertical)
        Report.result(Tests.location_changed, sc_main.dockWidgetArea(find_pane(sc_main, TEST_PANE_2)) == DOCKAREA)

        # 5) Resize test pane 3
        initial_size = test_pane_3.frameSize()
        test_pane_3.resize(initial_size.width() + SCALE_INT, initial_size.height() + SCALE_INT)
        new_size = test_pane_3.frameSize()
        resize_success = (
            abs(initial_size.width() - new_size.width()) == abs(initial_size.height() - new_size.height()) == SCALE_INT
        )
        Report.result(Tests.resize_pane_3, resize_success)

        # 6) Relaunch Script Canvas
        general.close_pane("Script Canvas")
        helper.wait_for_condition(lambda: general.is_pane_visible("Script Canvas"), 2.0)

        general.open_pane("Script Canvas")
        sc_visible = helper.wait_for_condition(lambda: general.is_pane_visible("Script Canvas"), 5.0)
        Report.result(Tests.relaunch_sc, sc_visible)

        # 7) Verify if test pane 1 retain its visibility
        editor_window = pyside_utils.get_editor_main_window()
        sc = editor_window.findChild(QtWidgets.QDockWidget, "Script Canvas")
        Report.result(Tests.visibility_retained, not find_pane(sc, TEST_PANE_1).isVisible())

        # 8) Verify if location of test pane 2 is retained
        sc_main = sc.findChild(QtWidgets.QMainWindow)
        Report.result(Tests.location_retained, sc_main.dockWidgetArea(find_pane(sc_main, TEST_PANE_2)) == DOCKAREA)

        # 9) Verify if size of test pane 3 is retained
        test_pane_3 = sc.findChild(QtWidgets.QDockWidget, TEST_PANE_3)
        retained_size = test_pane_3.frameSize()
        retain_success = retained_size != initial_size
        Report.result(Tests.size_retained, retain_success)

    finally:
        # 10) Restore default layout and close SC window
        general.open_pane("Script Canvas")
        helper.wait_for_condition(lambda: general.is_pane_visible("Script Canvas"), 5.0)
        sc = editor_window.findChild(QtWidgets.QDockWidget, "Script Canvas")
        click_menu_option(sc, "Restore Default Layout")
        sc.close()


if __name__ == "__main__":
    import ImportPathHelper as imports

    imports.init()
    from utils import Report

    Report.start_test(Pane_Default_RetainOnSCRestart)
