"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

https://testrail.agscollab.com/index.php?/tests/view/92568973
"""


# fmt: off
class Tests():
    open_sc_window                 = ("Script Canvas window is opened",                           "Failed to open Script Canvas window")
    variable_manager_opened        = ("VariableManager is opened successfully",                   "Failed to open VariableManager")
    boolean_pinned                 = ("Boolean is pinned",                                        "Boolean is not pinned, But it should be unpinned")
    boolean_unpinned               = ("Boolean is unpinned",                                      "Boolean is not unpinned, But it should be pinned")
    boolean_unpinned_after_reopen  = ("Boolean is unpinned after reopening create variable menu", "Boolean is not unpinned after reopening create variable menu")
# fmt: on


def Unpin_VariableManager():
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
     - This test file must be called from the Lumberyard Editor command terminal
     - Any passed and failed tests are written to the Editor.log file.
        Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    from PySide2 import QtWidgets
    import azlmbr.legacy.general as general

    import pyside_utils
    from utils import TestHelper as helper
    from utils import Report
    from PySide2.QtCore import Qt

    GENERAL_WAIT = 5.0  # seconds

    def find_pane(window, pane_name):
        return window.findChild(QtWidgets.QDockWidget, pane_name)

    def click_menu_option(window, option_text):
        action = pyside_utils.find_child_by_pattern(window, {"text": option_text, "type": QtWidgets.QAction})
        action.trigger()

    # 1) Open Script Canvas window (Tools > Script Canvas)
    general.idle_enable(True)
    general.open_pane("Script Canvas")
    is_sc_visible = helper.wait_for_condition(lambda: general.is_pane_visible("Script Canvas"), 15.0)
    Report.result(Tests.open_sc_window, is_sc_visible)

    # 2) Get the SC window object
    editor_window = pyside_utils.get_editor_main_window()
    sc = editor_window.findChild(QtWidgets.QDockWidget, "Script Canvas")
    sc_main = sc.findChild(QtWidgets.QMainWindow)

    # 3) Open Variable Manager in Script Canvas window
    pane = find_pane(sc, "VariableManager")
    if not pane.isVisible():
        click_menu_option(sc, "Variable Manager")
    pane = find_pane(sc, "VariableManager")
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
    result = helper.wait_for_condition(
        lambda: model_index.siblingAtColumn(0).data(Qt.DecorationRole) is not None, GENERAL_WAIT
    )
    Report.result(Tests.boolean_pinned, result)
    # Unpin Boolean and make sure Boolean is unpinned.
    pyside_utils.item_view_index_mouse_click(table_view, model_index.siblingAtColumn(0))
    result = helper.wait_for_condition(
        lambda: model_index.siblingAtColumn(0).data(Qt.DecorationRole) is None, GENERAL_WAIT
    )
    Report.result(Tests.boolean_unpinned, result)

    # 7) Close and Reopen Create Variable menu and make sure Boolean is unpinned after reopening Create Variable menu
    button.click()
    button.click()
    general.idle_wait(1.0)
    result = helper.wait_for_condition(
        lambda: model_index.siblingAtColumn(0).data(Qt.DecorationRole) is None, GENERAL_WAIT
    )
    Report.result(Tests.boolean_unpinned_after_reopen, result)

    # 8) Restore default layout and close SC window
    click_menu_option(sc, "Restore Default Layout")
    sc.close()


if __name__ == "__main__":
    import ImportPathHelper as imports

    imports.init()

    from utils import Report

    Report.start_test(Unpin_VariableManager)
