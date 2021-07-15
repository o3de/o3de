"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# fmt: off
class Tests():
    select_multiple_targets = ("Multiple targets are selected", "Multiple targets are not selected")
# fmt: on


GENERAL_WAIT = 0.5  # seconds


def Debugger_HappyPath_TargetMultipleGraphs():
    """
    Summary:
     Multiple Graphs can be targeted in the Debugger tool

    Expected Behavior:
     Multiple elected files can be checked for logging.
     Upon checking, checkboxes of the parent folders change to either full or partial check.

    Test Steps:
     1) Open Script Canvas window (Tools > Script Canvas)
     2) Get the SC window object
     3) Open Debugging Tool if not opened already
     4) Select Graphs tab under logging window
     5) Select multiple targets from levels and scriptcanvas
     6) Verify if multiple targets are selected
     7) Close Debugging window and Script Canvas window


    Note:
     - This test file must be called from the Open 3D Engine Editor command terminal
     - Any passed and failed tests are written to the Editor.log file.
        Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """
    from PySide2 import QtWidgets
    from PySide2.QtCore import Qt
    import azlmbr.legacy.general as general

    import pyside_utils
    from utils import TestHelper as helper
    from utils import Report

    # 1) Open Script Canvas window (Tools > Script Canvas)
    general.idle_enable(True)
    general.open_pane("Script Canvas")
    helper.wait_for_condition(lambda: general.is_pane_visible("Script Canvas"), 6.0)

    # 2) Get the SC window object
    editor_window = pyside_utils.get_editor_main_window()
    sc = editor_window.findChild(QtWidgets.QDockWidget, "Script Canvas")

    # 3) Open Debugging Tool if not opened already
    if (
        sc.findChild(QtWidgets.QDockWidget, "LoggingWindow") is None
        or not sc.findChild(QtWidgets.QDockWidget, "LoggingWindow").isVisible()
    ):
        action = pyside_utils.find_child_by_pattern(sc, {"text": "Debugging", "type": QtWidgets.QAction})
        action.trigger()
    logging_window = sc.findChild(QtWidgets.QDockWidget, "LoggingWindow")

    # 4) Select Graphs tab under logging window
    button = pyside_utils.find_child_by_pattern(logging_window, {"type": QtWidgets.QPushButton, "text": "Graphs"})
    button.click()

    # 5) Select multiple targets from levels and scriptcanvas
    graphs = logging_window.findChild(QtWidgets.QWidget, "graphsPage")
    tree = graphs.findChild(QtWidgets.QTreeView, "pivotTreeView")
    # Select the first child under levels
    level_model_index = pyside_utils.find_child_by_pattern(tree, "levels")
    level_child_index = pyside_utils.get_item_view_index(tree, 0, 0, level_model_index)
    tree.model().setData(level_child_index, 2, Qt.CheckStateRole)
    # Select the first child under scriptcanvas
    sc_model_index = pyside_utils.find_child_by_pattern(tree, "scriptcanvas")
    sc_child_index = pyside_utils.get_item_view_index(tree, 0, 0, sc_model_index)
    tree.model().setData(sc_child_index, 2, Qt.CheckStateRole)

    # 6) Verify if multiple targets are selected
    result = all([index.data(Qt.CheckStateRole) != 0 for index in (level_model_index, sc_model_index)])
    result = result and all([index.data(Qt.CheckStateRole) == 2 for index in (level_child_index, sc_child_index)])
    Report.result(Tests.select_multiple_targets, result)

    # 7) Close Debugging window and Script Canvas window
    logging_window.close()
    general.close_pane("Script Canvas")


if __name__ == "__main__":
    import ImportPathHelper as imports

    imports.init()
    from utils import Report

    Report.start_test(Debugger_HappyPath_TargetMultipleGraphs)
