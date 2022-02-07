"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from PySide2 import QtWidgets

import editor_python_test_tools.pyside_utils as pyside_utils
from editor_python_test_tools.utils import TestHelper as helper
from editor_python_test_tools.utils import Report

import azlmbr.legacy.general as general


# fmt: off
class Tests():
    new_graph   = "New graph created"
    save_prompt = "Save prompt opened as expected"
    close_graph = "Close button worked as expected"
# fmt: on


GENERAL_WAIT = 0.5  # seconds


class TestGraphClose_Default_SavePrompt:
    """
    Summary:
     The graph is closed when x button is clicked.
     Save Prompt is opened before closing.

    Expected Behavior:
     The Graph is closed.
     Upon closing the graph, User is prompted whether or not to save changes.

    Test Steps:
     1) Open Script Canvas window (Tools > Script Canvas)
     2) Get the SC window object
     3) Trigger File->New action
     4) Verify if New tab is opened
     5) Close new tab using X on top of graph and check for save dialog
     6) Check if tab is closed
     7) Close Script Canvas window

    Note:
     - This test file must be called from the Open 3D Engine Editor command terminal
     - Any passed and failed tests are written to the Editor.log file.
        Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    @pyside_utils.wrap_async
    async def run_test(self):
        # 1) Open Script Canvas window (Tools > Script Canvas)
        general.idle_enable(True)
        general.open_pane("Script Canvas")
        helper.wait_for_condition(lambda: general.is_pane_visible("Script Canvas"), 5.0)

        # 2) Get the SC window object
        editor_window = pyside_utils.get_editor_main_window()
        sc = editor_window.findChild(QtWidgets.QDockWidget, "Script Canvas")
        sc_main = sc.findChild(QtWidgets.QMainWindow)
        sc_tabs = sc_main.findChild(QtWidgets.QTabWidget, "ScriptCanvasTabs")
        tab_bar = sc_tabs.findChild(QtWidgets.QTabBar)

        # 3) Trigger File->New action
        initial_tabs_count = sc_tabs.count()
        action = pyside_utils.find_child_by_pattern(
            sc_main, {"objectName": "action_New_Script", "type": QtWidgets.QAction}
        )
        action.trigger()

        # 4) Verify if New tab is opened
        result = helper.wait_for_condition(lambda: sc_tabs.count() == initial_tabs_count + 1, GENERAL_WAIT)
        Report.info(f"{Tests.new_graph}: {result}")

        # 5) Close new tab using X on top of graph and check for save dialog
        close_button = tab_bar.findChildren(QtWidgets.QAbstractButton)[0]
        pyside_utils.click_button_async(close_button)
        popup = await pyside_utils.wait_for_modal_widget()
        if popup:
            Report.info(f"{Tests.save_prompt}: {popup.findChild(QtWidgets.QDialog, 'SaveChangesDialog') is not None}")
            dont_save = popup.findChild(QtWidgets.QPushButton, "m_continueButton")
            dont_save.click()

        # 6) Check if tab is closed
        await pyside_utils.wait_for_condition(lambda: sc_tabs.count() == initial_tabs_count, 5.0)
        Report.info(f"{Tests.close_graph}: {sc_tabs.count()==initial_tabs_count}")

        # 7) Close Script Canvas window
        general.close_pane("Script Canvas")


test = TestGraphClose_Default_SavePrompt()
test.run_test()
