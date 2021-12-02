"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# fmt: off
class Tests():
    set_search_string     = ("Search string is set",              "Search string is not set")
    search_string_deleted = ("Search string deleted as expected", "Search string not deleted")
# fmt: on


def NodePalette_SearchText_Deletion():
    """
    Summary:
     We enter some string in the Node Palette Search box, select that text and delete it.

    Expected Behavior:
     After RightClick->Delete the text in the Searchbox should be deleted.

    Test Steps:
     1) Open Script Canvas window (Tools > Script Canvas)
     2) Get the SC window object
     3) Open Node Manager if not opened already
     4) Set some string in the Search box
     5) Verify if the test string is set
     6) Delete search string using right click and verify if it is cleared

    Note:
     - This test file must be called from the Open 3D Engine Editor command terminal
     - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    from PySide2 import QtWidgets, QtTest, QtCore

    from utils import TestHelper as helper

    import azlmbr.legacy.general as general

    import pyside_utils

    TEST_STRING = "TestString"

    # 1) Open Script Canvas window (Tools > Script Canvas)
    general.idle_enable(True)
    general.open_pane("Script Canvas")
    helper.wait_for_condition(lambda: general.is_pane_visible("Script Canvas"), 3.0)

    # 2) Get the SC window object
    editor_window = pyside_utils.get_editor_main_window()
    sc = editor_window.findChild(QtWidgets.QDockWidget, "Script Canvas")

    # 3) Open Node Manager if not opened already
    if sc.findChild(QtWidgets.QDockWidget, "NodePalette") is None:
        action = pyside_utils.find_child_by_pattern(sc, {"text": "Node Palette", "type": QtWidgets.QAction})
        action.trigger()
    node_palette = sc.findChild(QtWidgets.QDockWidget, "NodePalette")
    search_frame = node_palette.findChild(QtWidgets.QFrame, "searchFrame")

    # 4) Set some string in the Search box
    search_box = search_frame.findChild(QtWidgets.QLineEdit, "searchFilter")
    search_box.setText(TEST_STRING)

    # 5) Verify if the test string is set
    result = helper.wait_for_condition(lambda: search_box.text() == TEST_STRING, 1.0)
    Report.result(Tests.set_search_string, result)

    # 6) Delete search string using right click and verify if it is cleared
    QtTest.QTest.keyClick(search_box, QtCore.Qt.Key_A, QtCore.Qt.ControlModifier)
    pyside_utils.trigger_context_menu_entry(search_box, "Delete")
    result = helper.wait_for_condition(lambda: search_box.text() == "", 2.0)
    Report.result(Tests.search_string_deleted, result)


if __name__ == "__main__":
    import ImportPathHelper as imports

    imports.init()
    from utils import Report

    Report.start_test(NodePalette_SearchText_Deletion)
