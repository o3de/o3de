"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# fmt: off
class Tests():
    new_event_created   = ("New Script Event created",    "New Script Event not created")
    child_event_created = ("Child Event created",         "Child Event not created")
    params_added        = ("New parameters added",        "New parameters are not added")
    file_saved          = ("Script event file saved",     "Script event file did not save")
    node_found          = ("Node found in Script Canvas", "Node not found in Script Canvas")
# fmt: on


def ScriptEvents_AllParamDatatypes_CreationSuccess():
    """
    Summary:
     Parameters of all types can be created.

    Expected Behavior:
     The Method handles the large number of Parameters gracefully.
     Parameters of all data types can be successfully created.
     Updated ScriptEvent toast appears in Script Canvas.

    Test Steps:
     1) Open Asset Editor
     2) Initially create new Script Event file with one method
     3) Add new method and set name to it
     4) Add new parameters of each type
     5) Verify if parameters are added
     6) Expand the parameter rows
     7) Set different names and datatypes for each parameter
     8) Save file and verify node in SC Node Palette
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
    N_VAR_TYPES = 10  # Top 10 variable types
    TEST_METHOD_NAME = "test_method_name"

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
        for _ in range(20):
            QtTest.QTest.keyClick(search_box, QtCore.Qt.Key_Enter, QtCore.Qt.NoModifier)
            if pyside_utils.find_child_by_pattern(tree, {"text": node_name}) is not None:
                break

    def verify_added_params():
        for index in range(N_VAR_TYPES):
            if container.findChild(QtWidgets.QFrame, f"[{index}]") is None:
                return False
        return True

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

    # 3) Add new method and set name to it
    add_event = container.findChild(QtWidgets.QFrame, "Events").findChild(QtWidgets.QToolButton, "")
    add_event.click()
    result = helper.wait_for_condition(
        lambda: asset_editor_widget.findChild(QtWidgets.QFrame, "EventName") is not None, GENERAL_WAIT
    )
    Report.result(Tests.child_event_created, result)
    expand_container_rows("EventName")
    expand_container_rows("Name")
    initialize_asset_editor_qt_objects()
    children = container.findChildren(QtWidgets.QFrame, "Name")
    for child in children:
        line_edit = child.findChild(QtWidgets.QLineEdit)
        if line_edit is not None and line_edit.text() == "MethodName":
            line_edit.setText(TEST_METHOD_NAME)

    # 4) Add new parameters of each type
    helper.wait_for_condition(lambda: container.findChild(QtWidgets.QFrame, "Parameters") is not None, 2.0)
    parameters = container.findChild(QtWidgets.QFrame, "Parameters")
    add_param = parameters.findChild(QtWidgets.QToolButton, "")
    for _ in range(N_VAR_TYPES):
        add_param.click()

    # 5) Verify if parameters are added
    result = helper.wait_for_condition(verify_added_params, 3.0)
    Report.result(Tests.params_added, result)

    # 6) Expand the parameter rows (to render QFrame 'Type' for each param)
    for index in range(N_VAR_TYPES):
        expand_container_rows(f"[{index}]")

    # 7) Set different names and datatypes for each parameter
    expand_container_rows("Name")
    children = container.findChildren(QtWidgets.QFrame, "Name")
    index = 0
    for child in children:
        line_edit = child.findChild(QtWidgets.QLineEdit)
        if line_edit is not None and line_edit.text() == "ParameterName":
            line_edit.setText(f"param_{index}")
            index += 1

    children = container.findChildren(QtWidgets.QFrame, "Type")
    index = 0
    for child in children:
        combo_box = child.findChild(QtWidgets.QComboBox)
        if combo_box is not None and index < N_VAR_TYPES:
            combo_box.setCurrentIndex(index)
            index += 1

    # 8) Save file and verify node in SC Node Palette
    Report.result(Tests.file_saved, save_file())
    general.open_pane("Script Canvas")
    helper.wait_for_condition(lambda: general.is_pane_visible("Script Canvas"), 5.0)
    initialize_sc_qt_objects()
    node_palette_search(TEST_METHOD_NAME)
    get_node_index = lambda: pyside_utils.find_child_by_pattern(tree, {"text": TEST_METHOD_NAME}) is not None
    result = helper.wait_for_condition(get_node_index, 2.0)
    Report.result(Tests.node_found, result)

    # 9) Close Asset Editor
    general.close_pane("Asset Editor")
    general.close_pane("Script Canvas")


if __name__ == "__main__":
    import ImportPathHelper as imports

    imports.init()
    from utils import Report

    Report.start_test(ScriptEvents_AllParamDatatypes_CreationSuccess)
