"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Object to house all the Qt Objects and behavior used in testing the script canvas Node Inspector.

***Important Note: The QWidget hierarchy of the node inspector changes based on what node/variable is selected***
"""

from editor_python_test_tools.utils import TestHelper as helper
from PySide2 import QtWidgets
from editor_python_test_tools.QtPy.QtPyCommon import CheckBoxStates
from consts.scripting import (PROPERTY_EDITOR_QT, INITIAL_VALUE_SOURCE_QT, NODE_INSPECTOR_QT)
from consts.general import (WAIT_TIME_SEC_3)
from enum import IntEnum

class QtPyScriptCanvasNodeInspector():

    def __init__(self, sc_editor):
        self.node_inspector = sc_editor.sc_editor.findChild(QtWidgets.QDockWidget, NODE_INSPECTOR_QT)

    def change_variable_initial_value(self, variable_name: str, variable_type: str,  new_value) -> None:
        """
        Function for changing the initial value of a variable. This function will have branching behavior since variables
        can have many different types.

        Note: This function assumes the desired node is already in focus and visible to the node inspector

        IMPORTANT NOTE: Only the basic variable types are supported by this function. EntityId not supported at the moment
            Basic types: "Boolean", "Color", "Number", "String", "Transform", "Vector2", "Vector3", "Vector4"

        Basic type new_value structures
            Boolean:
            Color, String: String
            Number: float
            Transform: list[math.Vector3, math.Vector3, float]
            Vectors use corresponding math library vector

        params variable_name: the name of the variable that's selected
        params variable_type: the type of the variable being configured.
        params new_value: the new value you want to initialize the variable with

        returns None
        """

        variable_qobject = self.node_inspector.findChild(QtWidgets.QFrame, f"{variable_name}")

        assert variable_qobject is not None, f"Variable by name {variable_name} was not found."

        match variable_type:
            case "Boolean":
                assert type(new_value) == CheckBoxStates, f"New value: {new_value} is invalid for this variable type:{variable_type}"
                self.__change_variable_value_Boolean(variable_qobject, new_value)
            case "Color", "String":
                assert type(new_value) == str, f"New value: {new_value} is invalid for this variable type:{variable_type}"
                self.__change_variable_value_string(variable_qobject, new_value)
            case default:
                assert False, "An invalid variable type was provided"

    def __change_variable_value_Boolean(self, variable_qobject: QtWidgets.QFrame, new_value: IntEnum) -> None:
        """
        helper function for the change_variable initial_value branch (boolean variable type)
        """
        check_box = variable_qobject.findChild(QtWidgets.QCheckBox, "")
        new_value_to_bool = bool(new_value)

        if new_value_to_bool != check_box.isChecked():
            check_box.click()


    def __change_variable_value_string(self, variable_qobject: QtWidgets.QFrame, new_value: str) -> None:
        """
        helper function for the change_variable_initial_value branch(color, string variable type). This changes the
        variable's initial value to whatever is in the new_value argument. Color's RGBA value is a string and *NOT*
        a vector

        params variable_qobject: the qframe we need to drill into
        params new_value: the value we are replacing the old value with

        returns None
        """

        qline_edit = variable_qobject.findChild(QtWidgets.QLineEdit, "")
        helper.wait_for_condition(lambda: qline_edit is not None, WAIT_TIME_SEC_3)

        qline_edit.setText(f"{new_value}")

    def change_variable_initial_value_source(self, value_source: IntEnum) -> None:
        """
        Function for changing the scope of a variable. if you change the value source to 'from component' the variable
        will be exposed the script canvas component.

        Note: This function assumes the desired node is already in focus and visible to the node inspector

        params value_source: the index of the selection from the combo box Off = from graph On = from component

        """
        # local variables aren't all necessary but extracting them makes getting to the data easier to understand
        variable_property_editor_frame = self.node_inspector.findChild(QtWidgets.QFrame, PROPERTY_EDITOR_QT)
        variable_initial_value_source_frame = variable_property_editor_frame.findChild(QtWidgets.QFrame,
                                                                                       INITIAL_VALUE_SOURCE_QT)
        value_initial_source_combo_box = variable_initial_value_source_frame.findChild(QtWidgets.QComboBox, "")

        value_initial_source_combo_box.setCurrentIndex(value_source)
