"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# fmt: off
class Tests():
    new_event_created = ("New Script Event created",             "New Script Event not created")
    child_1_created   = ("Initial Child Event created",          "Initial Child Event not created")
    child_2_created   = ("Second Child Event created",           "Second Child Event not created")
    file_saved        = ("Script event file saved",              "Script event file did not save")
    method_added      = ("Method added to scriptevent file",     "Method not added to scriptevent file")
    method_removed    = ("Method removed from scriptevent file", "Method not removed from scriptevent file")
# fmt: on


def ScriptEvent_AddRemoveMethod_UpdatesInSC():
    """
    Summary:
     Method can be added/removed to an existing .scriptevents file

    Expected Behavior:
     The Method is correctly added/removed to the asset, and Script Canvas nodes are updated accordingly.

    Test Steps:
     1) Open Asset Editor and Script Canvas windows
     2) Initially create new Script Event file with one method
     3) Verify if file is created and saved
     4) Add a new child element
     5) Update MethodNames and save file
     6) Verify if the new node exist in SC (search in node palette)
     7) Delete one method and save
     8) Verify if the node is removed in SC
     9) Close Asset Editor

    Note:
     - This test file must be called from the Open 3D Engine Editor command terminal
     - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """
    import os
    from utils import TestHelper as helper
    import pyside_utils

    # Open 3D Engine imports
    import azlmbr.legacy.general as general
    import azlmbr.editor as editor
    import azlmbr.bus as bus

    # Pyside imports
    from PySide2 import QtWidgets, QtTest, QtCore

    GENERAL_WAIT = 1.0  # seconds

    FILE_PATH = os.path.join("AutomatedTesting", "TestAssets", "test_file.scriptevents")
    METHOD_NAME = "test_method_name"

    editor_window = pyside_utils.get_editor_main_window()
    asset_editor = asset_editor_widget = container = menu_bar = None
    sc = node_palette = tree = search_frame = search_box = None

    def initialize_asset_editor_qt_objects():
        nonlocal asset_editor, asset_editor_widget, container, menu_bar
        asset_editor = editor_window.findChild(QtWidgets.QDockWidget, "Asset Editor")
        asset_editor_widget = asset_editor.findChild(QtWidgets.QWidget, "AssetEditorWindowClass")
        container = asset_editor_widget.findChild(QtWidgets.QWidget, "ContainerForRows")
        menu_bar = asset_editor_widget.findChild(QtWidgets.QMenuBar)

    def initialize_sc_qt_objects():
        nonlocal sc, node_palette, tree, search_frame, search_box
        sc = editor_window.findChild(QtWidgets.QDockWidget, "Script Canvas")
        if sc.findChild(QtWidgets.QDockWidget, "NodePalette") is None:
            action = pyside_utils.find_child_by_pattern(sc, {"text": "Node Palette", "type": QtWidgets.QAction})
            action.trigger()
        node_palette = sc.findChild(QtWidgets.QDockWidget, "NodePalette")
        tree = node_palette.findChild(QtWidgets.QTreeView, "treeView")
        search_frame = node_palette.findChild(QtWidgets.QFrame, "searchFrame")
        search_box = search_frame.findChild(QtWidgets.QLineEdit, "searchFilter")

    def save_file():
        editor.AssetEditorWidgetRequestsBus(bus.Broadcast, "SaveAssetAs", FILE_PATH)
        action = pyside_utils.find_child_by_pattern(menu_bar, {"type": QtWidgets.QAction, "iconText": "Save"})
        action.trigger()
        # wait till file is saved, to validate that check the text of QLabel at the bottom of the AssetEditor,
        # if there are no unsaved changes we will not have any * in the text
        label = asset_editor.findChild(QtWidgets.QLabel, "textEdit")
        return helper.wait_for_condition(lambda: "*" not in label.text(), 3.0)

    def expand_container_rows(object_name):
        children = container.findChildren(QtWidgets.QFrame, object_name)
        for child in children:
            check_box = child.findChild(QtWidgets.QCheckBox)
            if check_box and not check_box.isChecked():
                QtTest.QTest.mouseClick(check_box, QtCore.Qt.LeftButton, QtCore.Qt.NoModifier)

    def node_palette_search(node_name):
        search_box.setText(node_name)
        helper.wait_for_condition(lambda: search_box.text() == node_name, 1.0)
        # Try clicking ENTER in search box multiple times
        for _ in range(10):
            QtTest.QTest.keyClick(search_box, QtCore.Qt.Key_Enter, QtCore.Qt.NoModifier)
            if pyside_utils.find_child_by_pattern(tree, {"text": node_name}) is not None:
                break

    # 1) Open Asset Editor
    general.idle_enable(True)
    # Initially close the Asset Editor and then reopen to ensure we don't have any existing assets open
    general.close_pane("Asset Editor")
    general.open_pane("Asset Editor")
    helper.wait_for_condition(lambda: general.is_pane_visible("Asset Editor"), 5.0)

    # 2) Initially create new Script Event file with one method
    initialize_asset_editor_qt_objects()
    action = pyside_utils.find_child_by_pattern(menu_bar, {"type": QtWidgets.QAction, "text": "Script Events"})
    action.trigger()
    result = helper.wait_for_condition(
        lambda: container.findChild(QtWidgets.QFrame, "Events") is not None
        and container.findChild(QtWidgets.QFrame, "Events").findChild(QtWidgets.QToolButton, "") is not None,
        3 * GENERAL_WAIT,
    )
    Report.result(Tests.new_event_created, result)
    # Add new method
    add_event = container.findChild(QtWidgets.QFrame, "Events").findChild(QtWidgets.QToolButton, "")
    add_event.click()
    result = helper.wait_for_condition(
        lambda: asset_editor_widget.findChild(QtWidgets.QFrame, "EventName") is not None, GENERAL_WAIT
    )
    Report.result(Tests.child_1_created, result)
    editor.AssetEditorWidgetRequestsBus(bus.Broadcast, "SaveAssetAs", FILE_PATH)

    # 3) Verify if file is created and saved
    result = helper.wait_for_condition(lambda: os.path.exists(FILE_PATH), 3 * GENERAL_WAIT)
    Report.result(Tests.file_saved, result and save_file())

    # 4) Add a new child element
    add_event = container.findChild(QtWidgets.QFrame, "Events").findChild(QtWidgets.QToolButton, "")
    add_event.click()
    result = helper.wait_for_condition(
        lambda: len(asset_editor_widget.findChildren(QtWidgets.QFrame, "EventName")) == 2, 2 * GENERAL_WAIT
    )
    Report.result(Tests.child_2_created, result)

    # 5) Update MethodNames and save file, (update all Method names to make it easier to search in SC later)
    # Expand the EventName initially
    expand_container_rows("EventName")
    # Expand Name fields under it
    expand_container_rows("Name")
    count = 0  # 2 Method names will be updated Ex: test_method_name_0, test_method_name_1
    container = asset_editor_widget.findChild(QtWidgets.QWidget, "ContainerForRows")
    children = container.findChildren(QtWidgets.QFrame, "Name")
    for child in children:
        line_edit = child.findChild(QtWidgets.QLineEdit)
        if line_edit and line_edit.text() == "MethodName":
            line_edit.setText(f"{METHOD_NAME}_{count}")
            count += 1
    save_file()

    # 6) Verify if the new node exist in SC (search in node palette)
    general.open_pane("Script Canvas")
    helper.wait_for_condition(lambda: general.is_pane_visible("Script Canvas"), 5.0)
    initialize_sc_qt_objects()
    node_palette_search(f"{METHOD_NAME}_1")
    get_node_index = lambda: pyside_utils.find_child_by_pattern(tree, {"text": f"{METHOD_NAME}_1"}) is not None
    result = helper.wait_for_condition(get_node_index, GENERAL_WAIT)
    Report.result(Tests.method_added, result)

    # 7) Delete one method and save
    initialize_asset_editor_qt_objects()
    for child in container.findChildren(QtWidgets.QFrame, "EventName"):
        if child.findChild(QtWidgets.QToolButton, ""):
            child.findChild(QtWidgets.QToolButton, "").click()
            break
    save_file()

    # 8) Verify if the node is removed in SC (search in node palette)
    initialize_sc_qt_objects()
    node_palette_search(f"{METHOD_NAME}_0")
    get_node_index = lambda: pyside_utils.find_child_by_pattern(tree, {"text": f"{METHOD_NAME}_0"}) is None
    result = helper.wait_for_condition(get_node_index, GENERAL_WAIT)
    Report.result(Tests.method_removed, result)

    # 9) Close Asset Editor
    general.close_pane("Asset Editor")
    general.close_pane("Script Canvas")


if __name__ == "__main__":
    import ImportPathHelper as imports

    imports.init()
    from utils import Report

    Report.start_test(ScriptEvent_AddRemoveMethod_UpdatesInSC)
