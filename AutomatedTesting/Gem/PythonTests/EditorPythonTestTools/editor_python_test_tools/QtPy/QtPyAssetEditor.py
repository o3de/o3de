"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Object to house all the Qt Objects and behavior used in testing the asset editor
"""

from editor_python_test_tools.utils import TestHelper as helper
from PySide2 import QtWidgets, QtCore
import pyside_utils
import azlmbr.editor as editor
import azlmbr.bus as bus
import os
from editor_python_test_tools.QtPy.QtPyCommon import QtPyCommon
from consts.asset_editor import (ASSET_EDITOR_UI, EVENTS_QT, DEFAULT_SCRIPT_EVENT, DEFAULT_METHOD_NAME)
from consts.general import (WAIT_TIME_SEC_3, SAVE_STRING, NAME_STRING)
from consts.scripting import (PARAMETERS_QT, INDICATOR_QT)



class QtPyAssetEditor(QtPyCommon):
    """
    QtPy class for manipulating the Asset editor UI and performing actions such as creating Script Events and
    saving them to disk.
    """

    def __init__(self, editor_main_window: QtWidgets.QMainWindow):
        super().__init__()
        self.asset_editor = editor_main_window.findChild(QtWidgets.QDockWidget, ASSET_EDITOR_UI)
        self.asset_editor_widget = self.asset_editor.findChild(QtWidgets.QWidget, "AssetEditorWindowClass")
        self.asset_editor_row_container = self.asset_editor_widget.findChild(QtWidgets.QWidget, "ContainerForRows")
        self.menu_bar = self.asset_editor_widget.findChild(QtWidgets.QMenuBar)

    def add_method_to_script_event(self, method_name: str) -> None:
        """
        function for adding a method to a script event. This mimics clicking the + button on the right side of the UI
        then entering text into the Name field.

        params method_name: The name you want to give the new method

        returns none
        """
        self.add_event_to_script_event()

        helper.wait_for_condition(lambda: self.asset_editor_widget.findChild(
            QtWidgets.QFrame, DEFAULT_SCRIPT_EVENT) is not None, WAIT_TIME_SEC_3)

        self.expand_qt_container_rows(DEFAULT_SCRIPT_EVENT)
        self.expand_qt_container_rows(NAME_STRING)
        self.update_new_method_name(DEFAULT_METHOD_NAME, method_name)

    def add_parameter_to_method(self, new_parameter_name: str, parameter_index = 0):
        """
        function for adding a new parameter to the script event method. This will need to be updated to support
        adding parameters to specific events or adding multiple parameters to a method.

        returns reference to the parameter's qtwidget
        """
        add_parameters_button = self.asset_editor_row_container.findChild(QtWidgets.QFrame, PARAMETERS_QT).findChild(
            QtWidgets.QToolButton, "")
        add_parameters_button.click()

        self.__refresh_qt_references()

        self.update_parameter_name("ParameterName", new_parameter_name, parameter_index)

    def change_parameter_type(self, parameter_index: int, type_index: int)-> None:
        """
        function for changing the data type of the script event method's parameters

        params parameter_index: the index of the method's parameters to target
        params type_index: the index of the data type to change the parameter to
        """

        type_qframes = self.asset_editor_row_container.findChildren(QtWidgets.QFrame, "Type")

        type_combo_boxes = []

        for qframe in type_qframes:
            combo_box = qframe.findChild(QtWidgets.QComboBox)
            if combo_box:
                type_combo_boxes.append(combo_box)

        target_combo_box = type_combo_boxes[parameter_index]
        target_combo_box.setCurrentIndex(type_index)

        set_type = target_combo_box.currentIndex()

        assert set_type is type_index, "Failed to set parameter type index."

    def update_parameter_name(self, old_parameter_name: str,updated_parameter_name: str, parameter_index: int) -> None:
        """
        helper function for updating the name of a parameter in the list of method parameters. This also needs to
        be updated if we are to support multiple parameters

        param old_parameter_name: the name of the parameter to update
        param updated_parameter_name: the new name you want the parameter to have

        returns: None

        """
        self.expand_qt_container_rows(f"[{parameter_index}]")
        self.expand_qt_container_rows(NAME_STRING)

        line_edits = self.asset_editor_row_container.findChildren(QtWidgets.QLineEdit)

        old_parameter_exists = False

        for line_edit in line_edits:
            if line_edit.text() == old_parameter_name:
                old_parameter_exists = True
                line_edit.setText(f"{updated_parameter_name}")
                break

        assert old_parameter_exists, f"Failed to find parameter {old_parameter_name} in Asset Editor."

    def update_new_method_name(self, old_method_name: str, updated_method_name: str) -> None:
        """
        Function for finding a default script event and changing its name

        param: method_name: the name you want to give the new method.

        Returns: None
        """
        children = self.asset_editor_row_container.findChildren(QtWidgets.QFrame, "Name")

        method_name_field = ""
        old_method_exists = False

        for child in children:

            line_edit = child.findChild(QtWidgets.QLineEdit)

            if line_edit and line_edit.text() == old_method_name:
                old_method_exists = True
                line_edit.setText(f"{updated_method_name}")
                method_name_field = line_edit.text()

        assert old_method_exists, f"Failed to find method {old_method_name} in Asset Editor."
        assert method_name_field == updated_method_name, "Failed to set method name in Asset Editor."

    def delete_parameter_from_method(self, parameter_index: int) -> None:
        """
        function for deleting a parameter from a script event at an index.

        param parameter_index: the index of the parameter to delete
        """

        parameter_qframe = self.asset_editor_row_container.findChild(QtWidgets.QFrame, f"[{parameter_index}]")

        toolbuttons = parameter_qframe.findChildren(QtWidgets.QToolButton)
        for button in toolbuttons:
            if button.objectName() != INDICATOR_QT:
                button.click()

        assert toolbuttons is not None, "Failed to delete parameter from Script Event method."

    def delete_method_from_script_events(self, ) -> None:
        """
        Function for deleting a method from an open script event file.

        params method_name: The name of the method you want to remove.

        returns None

        Note: I want to make this function seek out the script event function by name. Unfortunately, the list of Qobjects
        isn't organized in a way to make it easy to find and is regenerated every time a new method is added, preventing
        us from naming/maintaining a handle on a specific object.

        The name of the delete button is also bugged. It should have something more meaningful than  "".
        Refer to github issue #12262. If it has been resolved and this comment still exists please update
        the QToolButton name
        """
        methods = self.asset_editor_row_container.findChildren(QtWidgets.QFrame, DEFAULT_SCRIPT_EVENT)
        for method in methods:
            if method.findChild(QtWidgets.QToolButton, ""):
                method.findChild(QtWidgets.QToolButton, "").click()

    def save_script_event_file(self, file_path: str) -> None:
        """
        Function for saving the currently open script event file to disk then verifying the save action occurred

        param file_path: The path on disk to save the file to

        returns: None
        """
        editor.AssetEditorWidgetRequestsBus(bus.Broadcast, "SaveAssetAs", file_path)
        action = pyside_utils.find_child_by_pattern(self.menu_bar,
                                                    {"type": QtWidgets.QAction, "iconText": SAVE_STRING})

        assert action is not None, "Unable to complete save action. Filepath may be invalid."

        action.trigger()
        self.__verify_save_action(file_path)

    def __verify_save_action(self, file_path: str) -> None:
        """
        helper function to make the save_script_event_file function a bit cleaner. Performs verification that
        the save action occurred by checking the tab's label for the * and checking the location on disk for a file

        param file_path: The location of the created file on disk

        returns: None
        """
        label = self.asset_editor.findChild(QtWidgets.QLabel, "textEdit")
        saved = helper.wait_for_condition(lambda: "*" not in label.text(), WAIT_TIME_SEC_3)
        exists = helper.wait_for_condition(lambda: os.path.exists(file_path), WAIT_TIME_SEC_3)

        assert saved, "Save file failed. Save action not detected in UI (* in label)."
        assert exists, "Save file failed. File not located on disk."

    def expand_qt_container_rows(self, category_name: str) -> None:
        """
        calls the parent class function but passes in the asset_editor_row_container we have a handle on from init

        param category_name: the category in the list you want to expand.

        returns: None
        """
        super().expand_qt_container_rows(self.asset_editor_row_container, category_name)

    def __refresh_qt_references(self):
        """
        Function for updating references to qt objects associated with the asset manager. Expanding/deleting stuff
        from the list will cause the hierarchy to shift around so references can get stale, which makes interacting
        with it unpredictable

        returns: None
        """
        self.asset_editor_widget = self.asset_editor.findChild(QtWidgets.QWidget, "AssetEditorWindowClass")
        helper.wait_for_condition(lambda: self.asset_editor_widget is not None, WAIT_TIME_SEC_3)
        self.asset_editor_row_container = self.asset_editor_widget.findChild(QtWidgets.QWidget, "ContainerForRows")
        helper.wait_for_condition(lambda: self.asset_editor_row_container is not None, WAIT_TIME_SEC_3)

    def add_event_to_script_event(self):
        """
        Function for clicking the "+" button to add a script event to the script event asset. Also refreshes our
        reference to the asset editor qt objects

        returns None
        """
        add_event = self.asset_editor.findChild(QtWidgets.QFrame, EVENTS_QT).findChild(
            QtWidgets.QToolButton, "")

        assert add_event is not None, "Failed to add new method to script event."

        add_event.click()
        # refresh our handle on the asset editor widget since the qt object structure has changed
        self.__refresh_qt_references()
        
