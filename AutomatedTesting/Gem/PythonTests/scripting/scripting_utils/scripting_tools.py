"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
from editor_python_test_tools.utils import TestHelper as helper
from editor_python_test_tools.utils import Report
from PySide2 import QtWidgets, QtTest, QtCore
import editor_python_test_tools.pyside_utils as pyside_utils
import azlmbr.legacy.general as general
import azlmbr.editor as editor
import azlmbr.bus as bus
from scripting_utils.scripting_constants import (SCRIPT_CANVAS_UI, ASSET_EDITOR_UI, SCRIPT_EVENT_UI, NODE_PALETTE_UI,
                                                 NODE_PALETTE_QT, TREE_VIEW_QT, EVENTS_QT, DEFAULT_METHOD_NAME,
                                                 SEARCH_FRAME_QT, SEARCH_FILTER_QT, EVENT_NAME_QT, NAME_STRING,
                                                 PARAMETERS_QT, SAVE_STRING, SAVE_ASSET_AS, WAIT_TIME_3)


def save_file(self, file_path):
    editor.AssetEditorWidgetRequestsBus(bus.Broadcast, SAVE_ASSET_AS, file_path)
    action = pyside_utils.find_child_by_pattern(self.menu_bar, {"type": QtWidgets.QAction, "iconText": SAVE_STRING})
    action.trigger()
    # wait till file is saved, to validate that check the text of QLabel at the bottom of the AssetEditor,
    # if there are no unsaved changes we will not have any * in the text
    label = self.asset_editor.findChild(QtWidgets.QLabel, "textEdit")
    return helper.wait_for_condition(lambda: "*" not in label.text(), WAIT_TIME_3)


def initialize_asset_editor_qt_objects(self):
    self.editor_window = pyside_utils.get_editor_main_window()
    self.asset_editor = self.editor_window.findChild(QtWidgets.QDockWidget, ASSET_EDITOR_UI)
    self.asset_editor_widget = self.asset_editor.findChild(QtWidgets.QWidget, "AssetEditorWindowClass")
    self.asset_editor_row_container = self.asset_editor_widget.findChild(QtWidgets.QWidget, "ContainerForRows")
    self.menu_bar = self.asset_editor_widget.findChild(QtWidgets.QMenuBar)


def initialize_sc_qt_objects(self):
    self.script_canvas = self.editor_window.findChild(QtWidgets.QDockWidget, SCRIPT_CANVAS_UI)
    if self.script_canvas.findChild(QtWidgets.QDockWidget, NODE_PALETTE_QT) is None:
        action = pyside_utils.find_child_by_pattern(self.script_canvas, {"text": NODE_PALETTE_UI, "type": QtWidgets.QAction})
        action.trigger()
    self.node_palette = self.script_canvas.findChild(QtWidgets.QDockWidget, NODE_PALETTE_QT)
    self.node_tree_view = self.node_palette.findChild(QtWidgets.QTreeView, TREE_VIEW_QT)
    self.node_tree_search_frame = self.node_palette.findChild(QtWidgets.QFrame, SEARCH_FRAME_QT)
    self.none_tree_search_box = self.node_tree_search_frame.findChild(QtWidgets.QLineEdit, SEARCH_FILTER_QT)


def expand_qt_container_rows(self, object_name):
    children = self.asset_editor_row_container.findChildren(QtWidgets.QFrame, object_name)
    for child in children:
        check_box = child.findChild(QtWidgets.QCheckBox)
        if check_box and not check_box.isChecked():
            QtTest.QTest.mouseClick(check_box, QtCore.Qt.LeftButton, QtCore.Qt.NoModifier)


def canvas_node_palette_search(self, node_name, number_of_retries):
    self.none_tree_search_box.setText(node_name)
    helper.wait_for_condition(lambda: self.none_tree_search_box.text() == node_name, WAIT_TIME_3)
    # Try clicking ENTER in search box multiple times
    for _ in range(number_of_retries):
        QtTest.QTest.keyClick(self.none_tree_search_box, QtCore.Qt.Key_Enter, QtCore.Qt.NoModifier)
        if pyside_utils.find_child_by_pattern(self.node_tree_view, {"text": node_name}) is not None:
            break
