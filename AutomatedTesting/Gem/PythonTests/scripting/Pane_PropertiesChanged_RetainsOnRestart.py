"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    test_panes_visible = "All the test panes are opened"
    close_pane_1 = "Test pane 1 is closed"
    resize_pane_3 = "Test pane 3 resized successfully"
    location_changed = "Location of test pane 2 changed successfully"
    visiblity_retained = "Test pane retained its visiblity on Editor restart"
    location_retained = "Test pane retained its location on Editor restart"
    size_retained = "Test pane retained its size on Editor restart"


def Pane_PropertiesChanged_RetainsOnRestart():
    """
    Summary:
     The Script Canvas window is opened to verify if Script canvas panes can retain its visibility, size and location
     upon Editor restart.

    Expected Behavior:
     The ScriptCanvas pane retain it's visiblity, size and location upon Editor restart.

    Test Steps:
     1) Open Script Canvas window (Tools > Script Canvas)
     2) Make sure test panes are open and visible
     3) Close test pane 1
     4) Change dock location of test pane 2
     5) Resize test pane 3
     6) Restart Editor
     7) Verify if test pane 1 retain its visiblity
     8) Verify if location of test pane 2 is retained
     9) Verify if size of test pane 3 is retained
     10) Restore default layout and close SC window

    Note:
     - This test file must be called from the Open 3D Engine Editor command terminal
     - Any passed and failed tests are written to the Editor.log file.
        Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    import sys

    # Helper imports
    from utils import Report
    from utils import TestHelper as helper
    import pyside_utils

    # O3DE Imports
    import azlmbr.legacy.general as general

    # Pyside imports
    from PySide2 import QtCore, QtWidgets
    from PySide2.QtCore import Qt

    # Constants
    TEST_CONDITION = sys.argv[1]
    TEST_PANE_1 = "NodePalette"  # pane used to test  visibility
    TEST_PANE_2 = "VariableManager"  # pane used to test  location
    TEST_PANE_3 = "NodeInspector"  # pane used to test size
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
    helper.wait_for_condition(lambda: general.is_pane_visible("Script Canvas"), 5.0)

    if TEST_CONDITION == "before_restart":
        # 2) Make sure test panes are open and visible
        editor_window = pyside_utils.get_editor_main_window()
        sc = editor_window.findChild(QtWidgets.QDockWidget, "Script Canvas")
        click_menu_option(sc, "Restore Default Layout")
        test_pane_1 = sc.findChild(QtWidgets.QDockWidget, TEST_PANE_1)
        test_pane_2 = sc.findChild(QtWidgets.QDockWidget, TEST_PANE_2)
        test_pane_3 = sc.findChild(QtWidgets.QDockWidget, TEST_PANE_3)

        result = test_pane_1.isVisible() and test_pane_2.isVisible() and test_pane_3.isVisible()
        Report.info(f"{Tests.test_panes_visible}: {result}")

        # 3) Close test pane
        test_pane_1.close()
        Report.info(f"{Tests.close_pane_1}: {not test_pane_1.isVisible()}")

        # 4) Change dock location of test pane 2
        sc_main = sc.findChild(QtWidgets.QMainWindow)
        sc_main.addDockWidget(DOCKAREA, find_pane(sc_main, TEST_PANE_2), QtCore.Qt.Vertical)
        Report.info(f"{Tests.location_changed}: {sc_main.dockWidgetArea(find_pane(sc_main, TEST_PANE_2)) == DOCKAREA}")

        # 5) Resize test pane 3
        initial_size = test_pane_3.frameSize()
        test_pane_3.resize(initial_size.width() + SCALE_INT, initial_size.height() + SCALE_INT)
        new_size = test_pane_3.frameSize()
        resize_success = (
            abs(initial_size.width() - new_size.width()) == abs(initial_size.height() - new_size.height()) == SCALE_INT
        )
        Report.info(f"{Tests.resize_pane_3}: {resize_success}")

    if TEST_CONDITION == "after_restart":
        try:
            # 6) Restart Editor
            # Restart is not possible through script and hence it is done by running the same file as 2 tests with a
            # condition as before_test and after_test

            # 7) Verify if test pane 1 retain its visiblity
            # This pane closed before restart and expected that pane should not be visible.
            editor_window = pyside_utils.get_editor_main_window()
            sc = editor_window.findChild(QtWidgets.QDockWidget, "Script Canvas")
            Report.info(f"{Tests.visiblity_retained}: {not find_pane(sc, TEST_PANE_1).isVisible()}")

            # 8) Verify if location of test pane 2 is retained
            # This pane was set at DOCKAREA lcoation before restart
            sc_main = sc.findChild(QtWidgets.QMainWindow)
            Report.info(
                f"{Tests.location_retained}: {sc_main.dockWidgetArea(find_pane(sc_main, TEST_PANE_2)) == DOCKAREA}"
            )

            # 9) Verify if size of test pane 3 is retained
            # Verifying if size retained by checking current size not matching with default size
            test_pane_3 = find_pane(sc, TEST_PANE_3)
            retained_size = test_pane_3.frameSize()
            click_menu_option(sc, "Restore Default Layout")
            actual_size = test_pane_3.frameSize()
            Report.info(f"{Tests.size_retained}: {retained_size != actual_size}")

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

    Report.start_test(Pane_PropertiesChanged_RetainsOnRestart)
