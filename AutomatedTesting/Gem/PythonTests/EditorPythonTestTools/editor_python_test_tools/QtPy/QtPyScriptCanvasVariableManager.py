"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Object to house all the Qt Objects and behavior used in testing the script canvas variable manager
"""
from PySide2.QtCore import Qt
import types
from editor_python_test_tools.utils import TestHelper as helper
from PySide2 import QtWidgets, QtCore, QtTest
import pyside_utils
from types import SimpleNamespace
from consts.scripting import (VARIABLE_MANAGER_QT, VARIABLE_PALETTE_QT, ADD_BUTTON_QT, GRAPH_VARIABLES_QT, GRAPH_VARIABLES_PAGE_QT)
from consts.general import (WAIT_TIME_SEC_3)

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


class QtPyScriptCanvasVariableManager:
    """
    QtPy class for handling the behavior of the script canvas variable manager
    """

    def __init__(self, sc_editor):

        self.variable_manager = sc_editor.sc_editor.findChild(QtWidgets.QDockWidget, VARIABLE_MANAGER_QT)
        self.variable_types = self.get_basic_variable_types()

    def create_new_variable(self, variable_name: str, new_variable_type: str) -> QtCore.QItemSelection:
        """
        Function for adding a new variable to the variable manager's list

        param variable_name: the name you want to give the new variable
        param new_variable_type: the type of variable you want to create provided as a string. Is case-sensitive!

        returns: Item selection reference for the newly created variable
        """
        self.__validate_new_variable(new_variable_type)
        self.__click_on_add_variable_button()
        self.__select_and_verify_variable_type(new_variable_type)
        graph_vars_page = self.__set_new_variable_name(variable_name)
        selection = self.__get_variable_selection_index(graph_vars_page)

        return selection

    def __validate_new_variable(self, new_variable_type: str) -> None:
        """
        function for checking the provided variable type validity

        returns: None
        """
        assert type(new_variable_type) is str, "Wrong parameter type provided. new_variable_type was not str"
        assert new_variable_type in VARIABLE_TYPES_DICT, \
            "Wrong type provided. new_variable_type is not a valid type"

    def __click_on_add_variable_button(self) -> None:
        """
        helper function for adding a new variable. Clicks on the 'Create Variable' button and waits for the UI to render

        returns None
        """
        add_new_variable_button = self.variable_manager.findChild(QtWidgets.QPushButton, ADD_BUTTON_QT)
        add_new_variable_button.click()  # Click on Create Variable button

        table_view_generated = helper.wait_for_condition((
            lambda: self.variable_manager.findChild(QtWidgets.QTableView, VARIABLE_PALETTE_QT) is not None),
            WAIT_TIME_SEC_3)

        assert table_view_generated, "Variable list not generated after clicking Create Variable button"

    def __select_and_verify_variable_type(self, new_variable_type: str) -> None:
        """
        helper function for selecting a new variable type from the menu that appears when create variable button is clicked.
        function also extracts some other info out of the variable manager UI to verify that the new variable has been
        added to the list of user created variables.

        params: The type of variable to create as a string. ie "Boolean"

        returns Qwidget with the graph variables that have been created. Used for further helper methods
        """
        # Extract the graph variables table so we can verify a variable has been added to the list after selection
        graph_vars_table = self.variable_manager.findChild(QtWidgets.QTableView, GRAPH_VARIABLES_QT)
        expected_number_of_variables = graph_vars_table.model().rowCount(QtCore.QModelIndex()) + 1

        # Find the variable type that matches what we want to select
        table_view = self.variable_manager.findChild(QtWidgets.QTableView, VARIABLE_PALETTE_QT)
        variable_entry = pyside_utils.find_child_by_pattern(table_view, new_variable_type)

        # Click on it to create variable and wait for the UI to respond
        pyside_utils.item_view_index_mouse_click(table_view, variable_entry)
        actual_number_of_variables = graph_vars_table.model().rowCount(QtCore.QModelIndex())
        variable_added = helper.wait_for_condition(lambda: (actual_number_of_variables == expected_number_of_variables),
                                                   WAIT_TIME_SEC_3)

        assert variable_added, "New variable not added to graph variables list."

    def __set_new_variable_name(self, new_variable_name: str) -> QtWidgets:
        """
        helper function for adding a new variable. sets the new variable's name. waits for the UI to respond afterwards

        returns graph vars page object for future usage
        """
        # Get the widget with the newly added graph variable (currently selected)
        graph_vars_page = self.variable_manager.findChild(QtWidgets.QWidget, GRAPH_VARIABLES_PAGE_QT)

        # Set the new variable's name. Wait 3 seconds for the UI to respond
        line_edit = graph_vars_page.findChild(QtWidgets.QLineEdit, "")
        line_edit.insert(new_variable_name)
        var_name_changed = helper.wait_for_condition(lambda: line_edit.text() == new_variable_name, WAIT_TIME_SEC_3)

        assert var_name_changed, "Failed to change the variable's name"

        return graph_vars_page

    def __get_variable_selection_index(self, graph_vars_page: QtWidgets.QWidget) -> QtCore.QItemSelection:
        """
        helper function for adding a new variable. gets the index object from the created graph variables widget then
        emulates clicking somewhere else on the UI to remove focus from the graph variables . waits for the UI to
        respond afterwards.

        returns selection Qobject
        """
        selection_model = graph_vars_page.findChild(QtCore.QItemSelectionModel)
        selection = selection_model.selection()
        selection_model.clear()
        has_selection = helper.wait_for_condition(lambda: selection_model.hasSelection() is False, WAIT_TIME_SEC_3)

        assert has_selection, "Failed to clear selection from Variable Manager"

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
        has_selection = helper.wait_for_condition(lambda: selection_model.hasSelection(), WAIT_TIME_SEC_3)

        assert has_selection, "Failed to get selection from Variable Manager table"

    def delete_variable(self, variable_name: str) -> None:
        """
        Function for deleting a variable from the variable manager. Function selects each entry in the table and
        looks for the provided name. If it's found the variable manager is given focus and then the delete key is pressed.

        params variable_name: The name of the variable

        returns None
        """

        variable_table_view = self.variable_manager.findChild(QtWidgets.QTableView, GRAPH_VARIABLES_QT)
        variables_table_model = variable_table_view.model()
        row_count = variables_table_model.rowCount(QtCore.QModelIndex())

        for row in range(row_count):
            name_index = variables_table_model.index(row, 0)
            data = variables_table_model.data(name_index)
            if data == variable_name:
                variable_table_view.selectRow(row)
                self.variable_manager.activateWindow()
                helper.wait_for_condition(lambda: self.variable_manager.isActiveWindow(), WAIT_TIME_SEC_3)
                QtTest.QTest.keyClick(self.variable_manager, Qt.Key_Delete, Qt.NoModifier)
                break

    def get_basic_variable_types(self) -> types.SimpleNamespace:
        """
        function for getting easy to use container of variable types out of the dictionary

        returns: simple namespace container that allows you to access the most common variable types (strings) using dot notation
        """

        return SimpleNamespace(**VARIABLE_TYPES_DICT)

    def get_variable_count(self) -> int:
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
        variable_table_view, pinned_toggle = self.__find_pinned_toggle_control(variable_type)

        assert pinned_toggle is not None, "Failed to find variable pinning qt widget"

        toggle_state_is_different = self.__toggle_the_pinned_control(variable_table_view, pinned_toggle)

        assert toggle_state_is_different, "Variable pin not successfully toggled"

    def __find_pinned_toggle_control(self, variable_type: str) -> tuple:
        """
        helper function for toggling the pinned variable type in the create variable menu.

        returns tuple with the variable view and toggle control in it
        """
        variable_table_view = self.variable_manager.findChild(QtWidgets.QTableView, VARIABLE_PALETTE_QT)
        variable_object = pyside_utils.find_child_by_pattern(variable_table_view, variable_type)

        toggle_index = 0
        pinned_toggle = variable_object.siblingAtColumn(toggle_index)

        return variable_table_view, pinned_toggle

    def __toggle_the_pinned_control(self, variable_table_view, pinned_toggle) -> bool:
        """
        helper function for toggling the pinned variable type. performs a mouse click on the toggle control and waits
        for the UI to respond

        returns the boolean comparison of the state of the toggle control after it's been toggled.
        """
        pinned_toggle_state_before = pinned_toggle.data(Qt.DecorationRole)

        # click the toggle then varify it's different from before state
        pyside_utils.item_view_index_mouse_click(variable_table_view, pinned_toggle)

        pinned_toggle_state_after = helper.wait_for_condition(lambda: pinned_toggle.data(Qt.DecorationRole) is None,
                                                              WAIT_TIME_SEC_3)

        return pinned_toggle_state_before != pinned_toggle_state_after
