"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Object to house all the Qt Objects and behavior used in testing the script canvas variable manager
"""
from PySide2.QtCore import Qt
from editor_python_test_tools.utils import TestHelper as helper
from PySide2 import QtWidgets, QtCore
import pyside_utils
from types import SimpleNamespace
from consts.scripting import (VARIABLE_MANAGER_QT, VARIABLE_PALETTE_QT, ADD_BUTTON_QT, GRAPH_VARIABLES_QT, GRAPH_VARIABLES_PAGE_QT)
from consts.general import (WAIT_TIME_SEC_1, WAIT_TIME_SEC_3)

"""
Basic variable types.
These are str-str kv pairings so they can be used with SimpleNamespace class easily or as an iterable type.  
"""
VARIABLE_TYPES_DICT = {
    "Boolean": "Boolean",
    "Color": "Color",
    "EntityId": "EntityId",
    "Number": "Number",
    "String": "String",
    "Transform": "Transform",
    "Vector2": "Vector2",
    "Vector3": "Vector3",
    "Vector4": "Vector4"
}


class QtPyScriptCanvasVariableManager():
    """
    QtPy class for handling the behavior of the script canvas variable manager
    """

    def __init__(self, sc_editor):

        self.variable_manager = sc_editor.sc_editor.findChild(QtWidgets.QDockWidget, VARIABLE_MANAGER_QT)
        self.variable_types = self.get_basic_variable_types()

    def __validate_new_variable(self, new_variable_type: str) -> None:
        """
        function for checking the provided variable type for validity

        returns: None
        """
        assert type(new_variable_type) is str, "Wrong parameter type provided. new_variable_type was not str"
        assert new_variable_type in VARIABLE_TYPES_DICT, \
            "Wrong type provided. new_variable_type is not a valid type"

    def create_new_variable(self, variable_name: str, new_variable_type: str) -> QtCore.QItemSelection:
        """
        Function for adding a new variable to the variable manager's list

        param variable_name: the name you want to give the new variable
        param new_variable_type: the type of variable you want to create provided as a string. Is case-sensitive!

        returns: Item selection reference for the newly created variable
        """
        self.__validate_new_variable(new_variable_type)

        add_new_variable_button = self.variable_manager.findChild(QtWidgets.QPushButton, ADD_BUTTON_QT)
        add_new_variable_button.click()  # Click on Create Variable button

        helper.wait_for_condition((
            lambda: self.variable_manager.findChild(QtWidgets.QTableView, VARIABLE_PALETTE_QT) is not None),
            WAIT_TIME_SEC_3)

        # Select variable type
        table_view = self.variable_manager.findChild(QtWidgets.QTableView, VARIABLE_PALETTE_QT)
        variable_entry = pyside_utils.find_child_by_pattern(table_view, new_variable_type)

        # todo
        # maybe put these 3 steps into a helper function
        # maybe assert here ?

        # Click on it to create variable and wait 1 second for the UI to respond
        pyside_utils.item_view_index_mouse_click(table_view, variable_entry)
        helper.wait_for_condition(lambda: True is False, WAIT_TIME_SEC_1)

        # Set the new variable's name. Wait 1 second for the UI to respond
        graph_vars_page = self.variable_manager.findChild(QtWidgets.QWidget, GRAPH_VARIABLES_PAGE_QT)
        line_edit = graph_vars_page.findChild(QtWidgets.QLineEdit, "")
        line_edit.insert(variable_name)
        helper.wait_for_condition(lambda: True is False, WAIT_TIME_SEC_1)

        # Store our selection so we can return it. Clear the selection and wait 1 second for the UI to respond
        selection_model = graph_vars_page.findChild(QtCore.QItemSelectionModel)
        selection = selection_model.selection()
        selection_model.clear()
        helper.wait_for_condition(lambda: True is False, WAIT_TIME_SEC_1)

        return selection

    def select_variable_from_table(self, item_selection: QtCore.QItemSelection) -> None:
        """
        Function for selecting a variable from the table of created graph variables

        params item_selection: the QtCore object reference to the variable in the table

        returns None

        """

        graph_vars_page = self.variable_manager.findChild(QtWidgets.QWidget, GRAPH_VARIABLES_PAGE_QT)
        selection_model = graph_vars_page.findChild(QtCore.QItemSelectionModel)
        selection_model.select(item_selection, QtCore.QItemSelectionModel.Select)
        helper.wait_for_condition(lambda: True is False, WAIT_TIME_SEC_1)

        assert selection_model.selection() == item_selection, "Unable to select variable from variable manager table"

    def get_basic_variable_types(self):
        """
        function for getting easy to use container of variable types out of the dictionary

        returns: simple namespace container that allows you to access the most common variable types (strings) using dot notation
        """

        return SimpleNamespace(**VARIABLE_TYPES_DICT)

    def get_variable_count(self):
        """
        function for retrieving the number of variables in the variable manager's list

        returns: the number of variables as an integer
        """
        graph_variables = self.variable_manager.findChild(QtWidgets.QTableView, GRAPH_VARIABLES_QT)
        row_count = graph_variables.model().rowCount(QtCore.QModelIndex())

        return row_count

    def validate_variable_count(self, expected) -> None:
        """
        function to check if the current number of variables in variable manager matches the user provided input

        param expected: the expected number of variables in the variable manager

        returns None
        """
        row_count = self.get_variable_count()
        result = helper.wait_for_condition(lambda: expected == row_count, WAIT_TIME_SEC_3)

        assert result, "Variable count did not match expected value."

    def toggle_pin_variable_button(self, variable_type: str) -> None:
        """
        Function for clicking the pin toggle that appears for variables in the create variable list

        param variable_type: The type of variable you want to pin to the top of the list

        returns None
        """

        variable_table_view = self.variable_manager.findChild(QtWidgets.QTableView, VARIABLE_PALETTE_QT)
        variable_object = pyside_utils.find_child_by_pattern(variable_table_view, variable_type)

        toggle_index = 0
        pinned_toggle = variable_object.siblingAtColumn(toggle_index)

        assert pinned_toggle is not None, "Failed to find variable pinning qt widget"

        pinned_toggle_state_before = pinned_toggle.data(Qt.DecorationRole)

        #click the toggle then varify it's different than the before state
        pyside_utils.item_view_index_mouse_click(variable_table_view, pinned_toggle)

        pinned_toggle_state_after = helper.wait_for_condition(lambda: pinned_toggle.data(Qt.DecorationRole) is None,
                                                              WAIT_TIME_SEC_3)
        assert pinned_toggle_state_before != pinned_toggle_state_after, "Variable pin not successfully toggled"
        