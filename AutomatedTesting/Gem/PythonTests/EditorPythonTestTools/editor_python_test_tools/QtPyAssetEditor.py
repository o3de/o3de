"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Object to house all the Qt Objects and behavior used in testing the asset editor
"""

from editor_python_test_tools.utils import TestHelper as helper
from PySide2 import QtWidgets, QtCore, QtTest
import pyside_utils
import azlmbr.editor as editor
import azlmbr.bus as bus
import os
from editor_python_test_tools.QtPyCommon import QtPyCommon
from consts.asset_editor import (ASSET_EDITOR_UI, EVENTS_QT, DEFAULT_SCRIPT_EVENT, DEFAULT_METHOD_NAME)
from consts.general import (WAIT_TIME_SEC_3, SAVE_STRING)


class QtPyAssetEditor(QtPyCommon):
    """
    QtPy class for handling the behavior of the Asset Editor
     """

    def __init__(self, editor_main_window: QtWidgets.QMainWindow):
        super().__init__()
        self.asset_editor = editor_main_window.findChild(QtWidgets.QDockWidget, ASSET_EDITOR_UI)
        self.asset_editor_widget = self.asset_editor.findChild(QtWidgets.QWidget, "AssetEditorWindowClass")
        self.asset_editor_row_container = self.asset_editor_widget.findChild(QtWidgets.QWidget, "ContainerForRows")
        self.menu_tool_bar = self.asset_editor_widget.findChild(QtWidgets.QMenuBar)

    def add_method_to_script_event(self, method_name: str) -> None:
        """
        function for adding a method to a script event. This mimics clicking the + button on the right side of the UI
        then entering text into the Name field.

        params method_name: The name you want to give the new method

        returns none
        """
        add_event = self.asset_editor_row_container.findChild(QtWidgets.QFrame, EVENTS_QT).findChild(
            QtWidgets.QToolButton, "")

        assert add_event is not None, "Failed to add new method to script event."

        add_event.click()

        helper.wait_for_condition(lambda: self.asset_editor_widget.findChild(
            QtWidgets.QFrame, DEFAULT_SCRIPT_EVENT) is not None, WAIT_TIME_SEC_3)

        # Categories need to be expanded before we can manipulate fields hidden within
        self.expand_category_by_name(DEFAULT_SCRIPT_EVENT)
        self.expand_category_by_name("Name")

        self.update_new_method_name(method_name)

    def update_new_method_name(self, method_name: str) -> None:
        """
        Function for finding a default script event and changing its name

        param: method_name: the name you want to give the new method.

        Returns: None
        Note: I might want to make this function more robust and seek out a method by name instead of just looking for
        methods with default names
        """
        children = self.asset_editor_row_container.findChildren(QtWidgets.QFrame, "Name")
        for child in children:
            line_edit = child.findChild(QtWidgets.QLineEdit)
            if line_edit and line_edit.text() == DEFAULT_METHOD_NAME:
                line_edit.setText(f"{method_name}")

    def delete_method_from_script_events(self) -> None:
        """
        Function for deleting a script from an open script event file.

        returns None

        Note: I want to make this function more useful by allowing the user to delete a specific method by name. I
        tried to do this but couldn't find a way to seek out a specific method then click its associated delete button.
        The Qobjects don't seemed to be organized in a way for you to easily drill down from one into the next nested
        object.
        """

        methods = self.asset_editor_row_container.findChildren(QtWidgets.QFrame, DEFAULT_SCRIPT_EVENT)
        for method in methods:
            if method.findChild(QtWidgets.QToolButton, ""):
                method.findChild(QtWidgets.QToolButton, "").click()
                break

    def save_script_event_file(self, file_path: str) -> None:
        """
        Function for saving the currently open script event file to disk.

        param file_path: The path on disk to save the file to

        returns: None
        """
        editor.AssetEditorWidgetRequestsBus(bus.Broadcast, "SaveAssetAs", file_path)
        action = pyside_utils.find_child_by_pattern(self.menu_tool_bar,
                                                    {"type": QtWidgets.QAction, "iconText": SAVE_STRING})
        assert action is not None, "Unable to complete save action. Filepath may be invalid."

        action.trigger()
        # wait until the file is saved. validate by checking for an * in the label text and we can see the file on disk
        label = self.asset_editor.findChild(QtWidgets.QLabel, "textEdit")
        saved = helper.wait_for_condition(lambda: "*" not in label.text(), WAIT_TIME_SEC_3)
        exists = helper.wait_for_condition(lambda: os.path.exists(file_path), WAIT_TIME_SEC_3)

        assert saved and exists, "Unable to complete save action. Action may have timed out (3 seconds)"

    def expand_category_by_name(self, category_name: str) -> None:
        """
        Note: I think I want to move this to the Common class and make the container you want to explore a parameter
        Function for expanding a category container in a list.

        param category_name: the category in the list you want to expand.

        returns: None
        """
        children = self.asset_editor_row_container.findChildren(QtWidgets.QFrame, category_name)
        for child in children:
            check_box = child.findChild(QtWidgets.QCheckBox)
            if check_box and not check_box.isChecked():
                QtTest.QTest.mouseClick(check_box, QtCore.Qt.LeftButton, QtCore.Qt.NoModifier)
