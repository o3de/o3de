"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
from editor_python_test_tools.utils import TestHelper as helper
from editor_python_test_tools.utils import Report
from PySide2 import QtWidgets
import pyside_utils
import azlmbr.legacy.general as general
import azlmbr.paths as paths
import scripting_utils.scripting_tools as tools
from scripting_utils.scripting_constants import (SCRIPT_CANVAS_UI, ASSET_EDITOR_UI, DEFAULT_METHOD_NAME, EVENT_NAME_QT,
                                                 NAME_STRING, WAIT_TIME_3, PARAMETER_NAME)


# fmt: off
class Tests():
    script_canvas_opened = ("Script canvas successfully opened",    "Script Canvas failed to open")
    asset_editor_opened = ("asset editor successfully opened",    "Asset editor failed to open")
    params_added        = ("New parameters added",        "New parameters are not added")
    file_saved          = ("Script event file saved",     "Script event file did not save")
    node_found          = ("Node found in Script Canvas", "Node not found in Script Canvas")
# fmt: on


FILE_PATH = os.path.join(paths.projectroot, "TestAssets", "test_file.scriptevents")
N_VAR_TYPES = 10  # Top 10 variable types
SEARCH_RETRY_ATTEMPTS = 20
TEST_METHOD_NAME = "test_method_name"

class TestScriptEvents_AllParamDatatypes_CreationSuccess():
    """
    Summary:
     Parameters of all types can be created.

    Expected Behavior:
     The Method handles the large number of Parameters gracefully.
     Parameters of all data types can be successfully created.
     Updated ScriptEvent toast appears in Script Canvas.

    Test Steps:
     1) Open Asset Editor
     2) Initially create new Script Event file with one method
     3) Add new method and set name to it
     4) Add new parameters of each type
     5) Verify if parameters are added
     6) Expand the parameter rows
     7) Set different names and datatypes for each parameter
     8) Save file and verify node in SC Node Palette
     9) Close Asset Editor

    Note:
     - This test file must be called from the Open 3D Engine Editor command terminal
     - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    def __init__(self):

        self.editor_main_window = None
        self.asset_editor = None
        self.asset_editor_widget = None
        self.asset_editor_row_container = None
        self.asset_editor_menu_bar = None
        self.sc_editor = None
        self.sc_editor_main_window = None
        self.node_palette = None
        self.node_tree_view = None
        self.node_tree_search_frame = None
        self.node_tree_search_box = None

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

    @pyside_utils.wrap_async
    async def run_test(self):

        # Preconditions
        general.idle_enable(True)

        # 1) Open Asset Editor
        # Initially close the Asset Editor and then reopen to ensure we don't have any existing assets open
        general.close_pane(ASSET_EDITOR_UI)
        result = tools.open_asset_editor()
        Report.result(Tests.asset_editor_opened, result)
        tools.initialize_editor_object(self)

        # 2) Initialize asset editor, script canvas and create new Script Event file with one method
        tools.initialize_asset_editor_object(self)
        result = tools.open_script_canvas()
        Report.result(Tests.script_canvas_opened, result)
        tools.initialize_sc_editor_objects(self)
        tools.create_script_event(self)

        # 3) Add new method and set name to it
        tools.expand_qt_container_rows(self, EVENT_NAME_QT)
        tools.expand_qt_container_rows(self, NAME_STRING)
        children = self.asset_editor_row_container.findChildren(QtWidgets.QFrame, NAME_STRING)
        for child in children:
            line_edit = child.findChild(QtWidgets.QLineEdit)
            if line_edit is not None and line_edit.text() == DEFAULT_METHOD_NAME:
                line_edit.setText(TEST_METHOD_NAME)

        # 4) Add new parameters of each type
        tools.add_empty_parameter_to_script_event(self, N_VAR_TYPES)

        # 5) Verify if parameters are added
        result = helper.wait_for_condition(lambda: self.verify_added_params(), WAIT_TIME_3)
        Report.result(Tests.params_added, result)

        # 6) Expand the parameter rows (to render QFrame 'Type' for each param)
        for index in range(N_VAR_TYPES):
            tools.expand_qt_container_rows(self, f"[{index}]")

        # 7) Set different names and datatypes for each parameter
        tools.expand_qt_container_rows(self, NAME_STRING)

        self.set_name_field_names()
        self.set_type_field_types()

        # 8) Save file and verify node in SC Node Palette
        Report.result(Tests.file_saved, tools.save_script_event_file(self, FILE_PATH))
        tools.open_node_palette(self)
        tools.initialize_node_palette_object(self)
        result = tools.canvas_node_palette_search(self, TEST_METHOD_NAME, SEARCH_RETRY_ATTEMPTS)
        Report.result(Tests.node_found, result)

        # 9) Close Asset Editor
        general.close_pane(ASSET_EDITOR_UI)
        general.close_pane(SCRIPT_CANVAS_UI)


test = TestScriptEvents_AllParamDatatypes_CreationSuccess()
test.run_test()
