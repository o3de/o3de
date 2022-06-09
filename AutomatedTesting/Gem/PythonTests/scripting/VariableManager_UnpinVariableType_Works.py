"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from PySide2 import QtWidgets
import azlmbr.legacy.general as general
import editor_python_test_tools.pyside_utils as pyside_utils
from editor_python_test_tools.utils import TestHelper as helper
from editor_python_test_tools.utils import Report
from PySide2.QtCore import Qt
from scripting_utils.scripting_constants import WAIT_TIME_3


# fmt: off
class Tests:
    variable_manager_opened         = ("VariableManager is opened successfully",                    "Failed to open VariableManager")
    variable_pinned                 = ("Variable is pinned",                                        "Variable is not pinned, But it should be unpinned")
    variable_unpinned               = ("Variable is unpinned",                                      "Variable is not unpinned, But it should be pinned")
    variable_unpinned_after_reopen  = ("Variable is unpinned after reopening create variable menu", "Variable is not unpinned after reopening create variable menu")
# fmt: on


class VariableManager_UnpinVariableType_Works:
    """
    Summary:
     Unpin variable types in create variable menu.
    Expected Behavior:
     The variable unpinned in create variable menu remains unpinned after reopening create variable menu.
    Test Steps:
     1) Open Script Canvas window (Tools > Script Canvas)
     2) Get the SC window object
     3) Open Variable Manager in Script Canvas window
     4) Create new graph
     5) Click on the Create Variable button in the Variable Manager
     6) Unpin Boolean by clicking the "Pin" icon on its left side
     7) Close and Reopen Create Variable menu and make sure Boolean is unpinned after reopening Create Variable menu
     8) Restore default layout and close SC window
    Note:
     - This test file must be called from the Open 3D Engine Editor command terminal
     - Any passed and failed tests are written to the Editor.log file.
        Parsing the file or running a log_monitor are required to observe the test results.
    :return: None
    """

    def find_pane(self, window, pane_name):
        return window.findChild(QtWidgets.QDockWidget, pane_name)

    def click_menu_option(self, window, option_text):
        action = pyside_utils.find_child_by_pattern(window, {"text": option_text, "type": QtWidgets.QAction})
        action.trigger()

    @pyside_utils.wrap_async
    async def run_test(self):

        # Preconditions
        general.idle_enable(True)

        # 1) Open Script Canvas window (Tools > Script Canvas)
        general.open_pane("Script Canvas")
        helper.wait_for_condition(lambda: general.is_pane_visible("Script Canvas"), WAIT_TIME_3)

        # 2) Get the SC window object
        editor_window = pyside_utils.get_editor_main_window()
        sc = editor_window.findChild(QtWidgets.QDockWidget, "Script Canvas")
        sc_main = sc.findChild(QtWidgets.QMainWindow)

        # 3) Open Variable Manager in Script Canvas window
        pane = self.find_pane(sc, "VariableManager")
        if not pane.isVisible():
            self.click_menu_option(sc, "Variable Manager")
        pane = self.find_pane(sc, "VariableManager")
        Report.result(Tests.variable_manager_opened, pane.isVisible())

        # 4) Create new graph
        create_new_graph = pyside_utils.find_child_by_pattern(
            sc_main, {"objectName": "action_New_Script", "type": QtWidgets.QAction}
        )
        create_new_graph.trigger()

        # 5) Click on the Create Variable button in the Variable Manager
        variable_manager = sc_main.findChild(QtWidgets.QDockWidget, "VariableManager")
        button = variable_manager.findChild(QtWidgets.QPushButton, "addButton")
        button.click()

        # 6) Unpin Boolean by clicking the "Pin" icon on its left side
        table_view = variable_manager.findChild(QtWidgets.QTableView, "variablePalette")
        model_index = pyside_utils.find_child_by_pattern(table_view, "Boolean")
        # Make sure Boolean is pinned
        is_boolean = model_index.siblingAtColumn(0)
        result = helper.wait_for_condition(lambda: is_boolean.data(Qt.DecorationRole) is not None, WAIT_TIME_3)
        Report.result(Tests.variable_pinned, result)
        # Unpin Boolean and make sure Boolean is unpinned.
        pyside_utils.item_view_index_mouse_click(table_view, is_boolean)
        result = helper.wait_for_condition(lambda: is_boolean.data(Qt.DecorationRole) is None, WAIT_TIME_3)
        Report.result(Tests.variable_unpinned, result)

        # 7) Close and Reopen Create Variable menu and make sure Boolean is unpinned after reopening Create Variable menu
        button.click()
        button.click()
        model_index = pyside_utils.find_child_by_pattern(table_view, "Boolean")
        result = helper.wait_for_condition(
            lambda: model_index.siblingAtColumn(0).data(Qt.DecorationRole) is None, WAIT_TIME_3
        )
        Report.result(Tests.variable_unpinned_after_reopen, result)

        # 8) Restore default layout and close SC window
        self.click_menu_option(sc, "Restore Default Layout")
        general.close_pane("Script Canvas")


test = VariableManager_UnpinVariableType_Works()
test.run_test()