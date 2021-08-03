"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# fmt: off
class Tests():
    default_visible = ("All the panes visible by default",  "One or more panes do not visible by default")
    open_panes      = ("All the Panes opened successfully", "Failed to open one or more panes")
    close_pane      = ("All the Panes closed successfully", "Failed to close one or more panes")
# fmt: on


def Pane_HappyPath_OpenCloseSuccessfully():
    """
    Summary:
     The Script Canvas window is opened to verify if Script Canvas panes can be opened and closed.

    Expected Behavior:
     The panes open and close successfully.

    Test Steps:
     1) Open Script Canvas window (Tools > Script Canvas)
     2) Restore default layout
     3) Verify if panes were opened by default
     4) Close the opened panes
     5) Open Script Canvas panes (Tools > <pane>)
     6) Restore default layout
     7) Close Script Canvas window

    Note:
     - This test file must be called from the Open 3D Engine Editor command terminal
     - Any passed and failed tests are written to the Editor.log file.
        Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper
    import editor_python_test_tools.pyside_utils as pyside_utils

    # Open 3D Engine Imports
    import azlmbr.legacy.general as general

    # Pyside imports
    from PySide2 import QtWidgets

    PANE_WIDGETS = ("NodePalette", "VariableManager")

    def click_menu_option(window, option_text):
        action = pyside_utils.find_child_by_pattern(window, {"text": option_text, "type": QtWidgets.QAction})
        action.trigger()

    def find_pane(window, pane_name):
        return window.findChild(QtWidgets.QDockWidget, pane_name)

    def is_pane_visible(window, pane_name):
        pane = find_pane(window, pane_name)
        return pane.isVisible()

    # Test starts here
    general.idle_enable(True)

    # 1) Open Script Canvas window (Tools > Script Canvas)
    general.open_pane("Script Canvas")
    helper.wait_for_condition(lambda: general.is_pane_visible("Script Canvas"), 5.0)

    # 2) Restore default layout
    editor_window = pyside_utils.get_editor_main_window()
    sc = editor_window.findChild(QtWidgets.QDockWidget, "Script Canvas")
    click_menu_option(sc, "Restore Default Layout")

    # 3) Verify if panes were opened by default
    PANES_VISIBLE = all(is_pane_visible(sc, pane) for pane in PANE_WIDGETS)
    Report.critical_result(Tests.default_visible, PANES_VISIBLE)

    # 4) Close the opened panes
    for item in PANE_WIDGETS:
        pane = sc.findChild(QtWidgets.QDockWidget, item)
        pane.close()
        if pane.isVisible():
            Report.info(f"Failed to close pane : {item}")

    PANES_VISIBLE = any(is_pane_visible(sc, pane) for pane in PANE_WIDGETS)
    Report.result(Tests.close_pane, not PANES_VISIBLE)

    # 5) Open Script Canvas panes (Tools > <pane>)
    click_menu_option(sc, "Node Palette")
    click_menu_option(sc, "Variable Manager")
    PANES_VISIBLE = helper.wait_for_condition(lambda: all(is_pane_visible(sc, pane) for pane in PANE_WIDGETS), 2.0)
    Report.result(Tests.open_panes, PANES_VISIBLE)

    # 6) Restore default layout
    # Needed this step to restore to default in case of test failure
    click_menu_option(sc, "Restore Default Layout")

    # 7) Close Script Canvas window
    sc.close()


if __name__ == "__main__":
    import ImportPathHelper as imports

    imports.init()
    from editor_python_test_tools.utils import Report

    Report.start_test(Pane_HappyPath_OpenCloseSuccessfully)
