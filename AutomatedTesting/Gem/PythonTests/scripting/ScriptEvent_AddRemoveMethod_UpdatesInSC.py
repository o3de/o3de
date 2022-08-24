"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import os
from PySide2 import QtWidgets, QtTest, QtCore
from editor_python_test_tools.utils import TestHelper as helper
from editor_python_test_tools.utils import Report
import pyside_utils
import azlmbr.legacy.general as general
import azlmbr.editor as editor
import azlmbr.bus as bus
import azlmbr.paths as paths
from scripting_utils.scripting_constants import (SCRIPT_CANVAS_UI, ASSET_EDITOR_UI, NODE_PALETTE_UI, SCRIPT_EVENT_UI,
                                                 NODE_PALETTE_QT, EVENTS_QT, NODE_TEST_METHOD, DEFAULT_SCRIPT_EVENT,
                                                 TREE_VIEW_QT, DEFAULT_METHOD_NAME, SAVE_STRING, WAIT_TIME_3)

class Tests():
    new_event_created = ("New Script Event created", "New Script Event not created")
    child_1_created = ("Initial Child Event created", "Initial Child Event not created")
    child_2_created = ("Second Child Event created", "Second Child Event not created")
    file_saved = ("Script event file saved", "Script event file did not save")
    method_added = ("Method added to scriptevent file", "Method not added to scriptevent file")
    method_removed = ("Method removed from scriptevent file", "Method not removed from scriptevent file")

FILE_PATH = os.path.join(paths.projectroot, "TestAssets", "test_file.scriptevents")
NUM_TEST_METHODS = 2


class TestScriptEvent_AddRemoveMethod_UpdatesInSC():
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

    def __init__(self):
        self.editor_window = None
        self.asset_editor = None
        self.asset_editor_widget = None
        self.asset_editor_row_container = None
        self.asset_editor_pulldown_menu = None
        self.script_canvas = None
        self.node_palette = None
        self.node_palette_tree = None
        self.node_palette_search_frame = None
        self.node_palette_search_box = None

    def initialize_asset_editor_qt_objects(self):
        self.asset_editor = self.editor_window.findChild(QtWidgets.QDockWidget, ASSET_EDITOR_UI)
        self.asset_editor_widget = self.asset_editor.findChild(QtWidgets.QWidget, "AssetEditorWindowClass")
        self.asset_editor_row_container = self.asset_editor_widget.findChild(QtWidgets.QWidget, "ContainerForRows")
        self.asset_editor_pulldown_menu = self.asset_editor_widget.findChild(QtWidgets.QMenuBar)

    def initialize_sc_qt_objects(self):
        self.script_canvas = self.editor_window.findChild(QtWidgets.QDockWidget, SCRIPT_CANVAS_UI)
        if self.script_canvas.findChild(QtWidgets.QDockWidget, NODE_PALETTE_QT) is None:
            action = pyside_utils.find_child_by_pattern(self.script_canvas, {"text": NODE_PALETTE_UI, "type": QtWidgets.QAction})
            action.trigger()
        self.node_palette = self.script_canvas.findChild(QtWidgets.QDockWidget, NODE_PALETTE_QT)
        self.node_palette_tree = self.node_palette.findChild(QtWidgets.QTreeView, TREE_VIEW_QT)
        self.node_palette_search_frame = self.node_palette.findChild(QtWidgets.QFrame, "searchFrame")
        self.node_palette_search_box = self.node_palette_search_frame.findChild(QtWidgets.QLineEdit, "searchFilter")

    def save_file(self):
        editor.AssetEditorWidgetRequestsBus(bus.Broadcast, "SaveAssetAs", FILE_PATH)
        action = pyside_utils.find_child_by_pattern(self.asset_editor_pulldown_menu, {"type": QtWidgets.QAction, "iconText": SAVE_STRING})
        action.trigger()
        # wait till file is saved, to validate that check the text of QLabel at the bottom of the AssetEditor,
        # if there are no unsaved changes we will not have any * in the text
        label = self.asset_editor.findChild(QtWidgets.QLabel, "textEdit")
        return helper.wait_for_condition(lambda: "*" not in label.text(), WAIT_TIME_3)

    def expand_container_rows(self, object_name):
        children = self.asset_editor_row_container.findChildren(QtWidgets.QFrame, object_name)
        for child in children:
            check_box = child.findChild(QtWidgets.QCheckBox)
            if check_box and not check_box.isChecked():
                QtTest.QTest.mouseClick(check_box, QtCore.Qt.LeftButton, QtCore.Qt.NoModifier)

    def node_palette_search(self, node_name):
        self.node_palette_search_box.setText(node_name)
        helper.wait_for_condition(lambda: self.node_palette_search_box.text() == node_name, WAIT_TIME_3)
        QtTest.QTest.keyClick(self.node_palette_search_box, QtCore.Qt.Key_Enter, QtCore.Qt.NoModifier)

    @pyside_utils.wrap_async
    async def run_test(self):

        # Preconditions
        general.idle_enable(True)

        # 1) Open Asset Editor
        # Initially close the Asset Editor and then reopen to ensure we don't have any existing assets open
        general.close_pane(ASSET_EDITOR_UI)
        general.open_pane(ASSET_EDITOR_UI)
        helper.wait_for_condition(lambda: general.is_pane_visible(ASSET_EDITOR_UI), WAIT_TIME_3)
        self.editor_window = pyside_utils.get_editor_main_window()

        # 2) Initially create new Script Event file with one method
        self.initialize_asset_editor_qt_objects()
        action = pyside_utils.find_child_by_pattern(self.asset_editor_pulldown_menu, {"type": QtWidgets.QAction, "text": SCRIPT_EVENT_UI})
        action.trigger()
        result = helper.wait_for_condition(
            lambda: self.asset_editor_row_container.findChild(QtWidgets.QFrame, EVENTS_QT) is not None
                    and self.asset_editor_row_container.findChild(QtWidgets.QFrame, EVENTS_QT).findChild(QtWidgets.QToolButton, "") is not None,
            WAIT_TIME_3,
        )
        Report.result(Tests.new_event_created, result)
        # Add new method
        add_event = self.asset_editor_row_container.findChild(QtWidgets.QFrame, EVENTS_QT).findChild(QtWidgets.QToolButton, "")
        add_event.click()
        result = helper.wait_for_condition(
            lambda: self.asset_editor_widget.findChild(QtWidgets.QFrame, DEFAULT_SCRIPT_EVENT) is not None, WAIT_TIME_3
        )
        Report.result(Tests.child_1_created, result)

        # 3) Verify if file is created and saved
        file_saved = self.save_file()
        result = helper.wait_for_condition(lambda: os.path.exists(FILE_PATH), WAIT_TIME_3)
        Report.result(Tests.file_saved, result and file_saved)

        # 4) Add a new child element
        add_event = self.asset_editor_row_container.findChild(QtWidgets.QFrame, EVENTS_QT).findChild(QtWidgets.QToolButton, "")
        add_event.click()
        result = helper.wait_for_condition(
            lambda: len(self.asset_editor_widget.findChildren(QtWidgets.QFrame, DEFAULT_SCRIPT_EVENT)) == NUM_TEST_METHODS,
            NUM_TEST_METHODS * WAIT_TIME_3
        )
        Report.result(Tests.child_2_created, result)

        # 5) Update MethodNames and save file, (update all Method names to make it easier to search in SC later)
        # Expand the EventName initially
        self.expand_container_rows(DEFAULT_SCRIPT_EVENT)
        # Expand Name fields under it
        self.expand_container_rows("Name")
        count = 0  # 2 Method names will be updated Ex: test_method_name_0, test_method_name_1
        container = self.asset_editor_widget.findChild(QtWidgets.QWidget, "ContainerForRows")
        children = container.findChildren(QtWidgets.QFrame, "Name")
        for child in children:
            line_edit = child.findChild(QtWidgets.QLineEdit)
            if line_edit and line_edit.text() == DEFAULT_METHOD_NAME:
                line_edit.setText(f"{NODE_TEST_METHOD}_{count}")
                count += 1
        self.save_file()

        # 6) Verify if the new node exist in SC (search in node palette)
        general.open_pane(SCRIPT_CANVAS_UI)
        helper.wait_for_condition(lambda: general.is_pane_visible(SCRIPT_CANVAS_UI), WAIT_TIME_3)
        self.initialize_sc_qt_objects()
        self.node_palette_search(f"{NODE_TEST_METHOD}_1")
        method_result_1 = helper.wait_for_condition(
            lambda: pyside_utils.find_child_by_pattern(self.node_palette_tree, {"text": f"{NODE_TEST_METHOD}_1"}) is not None, WAIT_TIME_3)
        Report.result(Tests.method_added, method_result_1)

        # 7) Delete one method and save
        self.initialize_asset_editor_qt_objects()
        for child in container.findChildren(QtWidgets.QFrame, DEFAULT_SCRIPT_EVENT):
            if child.findChild(QtWidgets.QToolButton, ""):
                child.findChild(QtWidgets.QToolButton, "").click()
                break
        self.save_file()

        # 8) Verify if the node is removed in SC (search in node palette)
        self.initialize_sc_qt_objects()
        self.node_palette_search(f"{NODE_TEST_METHOD}_0")
        method_result_0 = helper.wait_for_condition(
            lambda: pyside_utils.find_child_by_pattern(self.node_palette_tree, {"text": f"{NODE_TEST_METHOD}_0"}) is None, WAIT_TIME_3)
        Report.result(Tests.method_removed, method_result_0)

        # 9) Close Asset Editor
        general.close_pane(ASSET_EDITOR_UI)
        general.close_pane(SCRIPT_CANVAS_UI)


test = TestScriptEvent_AddRemoveMethod_UpdatesInSC()
test.run_test()
