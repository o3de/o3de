"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Object to house all the Qt Objects used when testing and manipulating the O3DE UI
"""
from editor_python_test_tools.utils import TestHelper as helper
from PySide2 import QtWidgets, QtCore
import pyside_utils
from scripting_utils.scripting_constants import (VARIABLE_MANAGER_QT, VARIABLE_TYPES, VARIABLE_PALETTE_QT,
                                                 ADD_BUTTON_QT, WAIT_TIME_3, GRAPH_VARIABLES_QT)
from editor_python_test_tools.utils import Report

class WrapperQTVariableManager:

    def __init__(self, sc_editor):
        self.variable_manager = sc_editor.findChild(QtWidgets.QDockWidget, VARIABLE_MANAGER_QT)

    def make_new_variable(self, new_variable_type):

        if type(new_variable_type) is not str:
            Report.critical_result(["Invalid variable type provided", ""], False)

        valid_type = False
        for this_type in VARIABLE_TYPES:
            if new_variable_type == this_type:
                valid_type = True

        if not valid_type:
            Report.critical_result(["Invalid variable type provided", ""], False)

        add_new_variable_button = self.variable_manager.findChild(QtWidgets.QPushButton, ADD_BUTTON_QT)
        add_new_variable_button.click()  # Click on Create Variable button
        helper.wait_for_condition((
            lambda: self.variable_manager.findChild(QtWidgets.QTableView, VARIABLE_PALETTE_QT) is not None), WAIT_TIME_3)
        # Select variable type
        table_view = self.variable_manager.findChild(QtWidgets.QTableView, VARIABLE_PALETTE_QT)
        model_index = pyside_utils.find_child_by_pattern(table_view, new_variable_type)
        # Click on it to create variable
        pyside_utils.item_view_index_mouse_click(table_view, model_index)

    def get_variable_count(self):

        graph_variables = self.variable_manager.findChild(QtWidgets.QTableView, GRAPH_VARIABLES_QT)
        row_count = graph_variables.model().rowCount(QtCore.QModelIndex())
        return row_count




