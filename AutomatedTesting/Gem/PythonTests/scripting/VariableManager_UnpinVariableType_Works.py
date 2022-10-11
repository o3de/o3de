"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from PySide2 import QtWidgets
import azlmbr.legacy.general as general
import pyside_utils
from editor_python_test_tools.utils import TestHelper as helper
from editor_python_test_tools.utils import Report
from PySide2.QtCore import Qt
import scripting_utils.scripting_tools as scripting_tools
from scripting_utils.scripting_constants import (WAIT_TIME_3, SCRIPT_CANVAS_UI, VARIABLE_PALETTE_QT, ADD_BUTTON_QT,
                                                 VARIABLE_TYPES, RESTORE_DEFAULT_LAYOUT)


# fmt: off
class Tests:
    variable_manager_opened         = ("VariableManager is opened successfully",                    "Failed to open VariableManager")
    variable_pinned                 = ("Variable is pinned",                                        "Variable is not pinned, But it should be unpinned")
    variable_unpinned               = ("Variable is unpinned",                                      "Variable is not unpinned, But it should be pinned")
    variable_unpinned_after_reopen  = ("Variable is unpinned after reopening create variable menu", "Variable is not unpinned after reopening create variable menu")
# fmt: on


BOOLEAN_STRING = VARIABLE_TYPES[0]

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
       - Any passed and failed tests are written to the Editor.log file.
        Parsing the file or running a log_monitor are required to observe the test results.
    :return: None
    """

    def __init__(self):
        self.editor_main_window = None
        self.sc_editor = None
        self.sc_editor_main_window = None
        self.variable_manager = None

    @pyside_utils.wrap_async
    async def run_test(self):

        # Preconditions
        general.idle_enable(True)

        # 1) Open Script Canvas window (Tools > Script Canvas)
        general.open_pane(SCRIPT_CANVAS_UI)
        helper.wait_for_condition(lambda: general.is_pane_visible(SCRIPT_CANVAS_UI), WAIT_TIME_3)

        # 2) Get the SC window object
        scripting_tools.initialize_editor_object(self)
        scripting_tools.initialize_sc_editor_objects(self)

        # 3) Open Variable Manager in Script Canvas window
        scripting_tools.initialize_variable_manager_object(self)
        Report.result(Tests.variable_manager_opened, self.variable_manager.isVisible())

        # 4) Create new graph
        scripting_tools.create_new_sc_graph(self.editor_main_window)

        # 5) Click on the Create Variable button in the Variable Manager
        create_variable_button = self.variable_manager.findChild(QtWidgets.QPushButton, ADD_BUTTON_QT)
        create_variable_button.click()

        # 6) Unpin Boolean by clicking the "Pin" icon on its left side
        variable_table_view = self.variable_manager.findChild(QtWidgets.QTableView, VARIABLE_PALETTE_QT)
        boolean_variable_object = pyside_utils.find_child_by_pattern(variable_table_view, BOOLEAN_STRING)
        # Make sure Boolean starts pinned
        pinned_toggle = boolean_variable_object.siblingAtColumn(0)
        result = helper.wait_for_condition(lambda: pinned_toggle.data(Qt.DecorationRole) is not None, WAIT_TIME_3)
        Report.result(Tests.variable_pinned, result)
        # Unpin Boolean and make sure Boolean is unpinned.
        pyside_utils.item_view_index_mouse_click(variable_table_view, pinned_toggle)
        result = helper.wait_for_condition(lambda: pinned_toggle.data(Qt.DecorationRole) is None, WAIT_TIME_3)
        Report.result(Tests.variable_unpinned, result)

        # 7) Close and Reopen Create Variable menu and make sure Boolean is unpinned after reopening Create Variable menu
        create_variable_button.click()
        create_variable_button.click()
        boolean_variable_object = pyside_utils.find_child_by_pattern(variable_table_view, BOOLEAN_STRING)
        result = helper.wait_for_condition(
            lambda: boolean_variable_object.siblingAtColumn(0).data(Qt.DecorationRole) is None, WAIT_TIME_3
        )
        Report.result(Tests.variable_unpinned_after_reopen, result)

        # 8) Restore default layout and close SC window
        boolean_variable_object = pyside_utils.find_child_by_pattern(variable_table_view, BOOLEAN_STRING)
        pinned_toggle = boolean_variable_object.siblingAtColumn(0)
        pyside_utils.item_view_index_mouse_click(variable_table_view, pinned_toggle)
        scripting_tools.click_menu_option(self.sc_editor, RESTORE_DEFAULT_LAYOUT)
        general.close_pane(SCRIPT_CANVAS_UI)


test = VariableManager_UnpinVariableType_Works()
test.run_test()
