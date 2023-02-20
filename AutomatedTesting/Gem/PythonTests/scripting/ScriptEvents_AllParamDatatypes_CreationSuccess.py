"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

N_VAR_TYPES = 10  # Top 10 variable types

def verify_added_params(self):
        """
        function for checking if there's enough parameter fields in the parameter container for all the parameter types we want to add

        returns true only if there's a row for each parameter we want to make
        """
        for index in range(N_VAR_TYPES):
            if self.asset_editor_row_container.findChild(QtWidgets.QFrame, f"[{index}]") is None:
                return False
        return True

def set_name_field_names(self):
        """
        function for assigning a name to each of the parameters name fields

        returns None
        """
        index = 0
        name_fields = tools.get_script_event_parameter_name_text(self)
        for name_field in name_fields:
            if name_field is not None and name_field.text() == PARAMETER_NAME:
                name_field.setText(f"param_{index}")
                index += 1

def set_type_field_types(self):
        """
        function for assigning a type to each of parameter's type combo box

        returns None
        """
        index = 0
        type_combo_boxes = tools.get_script_event_parameter_type_combobox(self)
        for type_combo_box in type_combo_boxes:
            if type_combo_box is not None and index < N_VAR_TYPES:
                type_combo_box.setCurrentIndex(index)
                index += 1

def TestScriptEvents_AllParamDatatypes_CreationSuccess():
    """
       Summary:
        Parameters of all types can be created.

       Expected Behavior:
        The Method handles the large number of Parameters gracefully.
        Parameters of all data types can be successfully created.
        Updated ScriptEvent toast appears in Script Canvas.

       Test Steps:
        1) Get a handle on our O3DE QtPy objects then initialize the Asset Editor object
        2) Create new Script Event Asset
        3) Add a method to the script event
        4) Add new parameters of each type
        5) Close Asset Editor

       Note:
        - Any passed and failed tests are written to the Editor.log file.
               Parsing the file or running a log_monitor are required to observe the test results.

       :return: None
       """
    import azlmbr.legacy.general as general
    from editor_python_test_tools.QtPy.QtPyO3DEEditor import QtPyO3DEEditor
    from scripting_utils.scripting_constants import (SCRIPT_CANVAS_UI, ASSET_EDITOR_UI)

    # Preconditions
    general.idle_enable(True)

    # 1) Get a handle on our O3DE QtPy objects then initialize the Asset Editor object
    qtpy_o3de_editor = QtPyO3DEEditor()
    # Close and reopen Asset Editor to ensure we don't have any existing assets open
    qtpy_o3de_editor.close_asset_editor()
    qtpy_asset_editor = qtpy_o3de_editor.open_asset_editor()

    # 2) Create new Script Event Asset
    qtpy_asset_editor.click_menu_bar_option("Script Events")

    # 3) Add a method to the script event
    qtpy_asset_editor.add_method_to_script_event("test_method")

    # 4) Add new parameters of each type
    for i in range(0, N_VAR_TYPES):
        qtpy_asset_editor.add_parameter_to_method(f"test_parameter_{i}", i)
        qtpy_asset_editor.change_parameter_type(i, i)

    # 5) Close Asset Editor
    general.close_pane(ASSET_EDITOR_UI)
    general.close_pane(SCRIPT_CANVAS_UI)


if __name__ == "__main__":

    import ImportPathHelper as imports

    imports.init()
    from editor_python_test_tools.utils import Report

    Report.start_test(TestScriptEvents_AllParamDatatypes_CreationSuccess)