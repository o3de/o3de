"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Object to house all the Qt Objects and behavior used in testing the script canvas variable manager
"""
from editor_python_test_tools.utils import TestHelper as helper
from PySide2 import QtWidgets, QtCore
import pyside_utils
from types import SimpleNamespace
from consts.scripting import (VARIABLE_MANAGER_QT, VARIABLE_PALETTE_QT, ADD_BUTTON_QT, GRAPH_VARIABLES_QT)
from consts.general import (WAIT_TIME_3)
from editor_python_test_tools.utils import Report


class Tests():
    variable_count_expected = ("Variable count matches expected", "Variable count does not match expected")


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


    def __validate_new_variable(self, new_variable_type):
        """
        function for checking the provided variable type for validity
        """
        if type(new_variable_type) is not str:
            Report.critical_result(["Invalid variable type provided", ""], False)

        valid_type = False
        for this_type in VARIABLE_TYPES_DICT:
            if new_variable_type == VARIABLE_TYPES_DICT[this_type]:
                valid_type = True

        if not valid_type:
            Report.critical_result(["Invalid variable type provided", ""], False)

    def create_new_variable(self, new_variable_type):
        """
        function for adding a new variable to the variable manager's list

        param new_variable_type: the type of variable you want to create provided as a string. Is case-sensitive!

        returns: None
        """
        self.__validate_new_variable(new_variable_type)

        add_new_variable_button = self.variable_manager.findChild(QtWidgets.QPushButton, ADD_BUTTON_QT)
        add_new_variable_button.click()  # Click on Create Variable button

        helper.wait_for_condition((
            lambda: self.variable_manager.findChild(QtWidgets.QTableView, VARIABLE_PALETTE_QT) is not None), WAIT_TIME_3)

        # Select variable type
        table_view = self.variable_manager.findChild(QtWidgets.QTableView, VARIABLE_PALETTE_QT)
        variable_entry = pyside_utils.find_child_by_pattern(table_view, new_variable_type)

        # Click on it to create variable
        pyside_utils.item_view_index_mouse_click(table_view, variable_entry)

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

    def validate_variable_count(self, expected):
        """
        function to check if the current number of variables in variable manager matches the user provided input

        param expected: the expected number of variables in the variable manager

        returns None
        """
        row_count = self.get_variable_count()
        result = expected == row_count

        Report.result(Tests.variable_count_expected, helper.wait_for_condition(lambda: result is True, WAIT_TIME_3))
