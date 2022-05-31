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
import azlmbr.paths as paths
import scripting_utils.scripting_tools as tools
from scripting_utils.scripting_constants import (SCRIPT_CANVAS_UI, ASSET_EDITOR_UI, SCRIPT_EVENT_UI, EVENTS_QT,
                                                 DEFAULT_METHOD_NAME, EVENT_NAME_QT, NAME_STRING, PARAMETERS_QT, WAIT_TIME_3)


# fmt: off
class Tests():
    new_event_created   = ("New Script Event created",    "New Script Event not created")
    child_event_created = ("Child Event created",         "Child Event not created")
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

        editor_window = None
        asset_editor = None
        asset_editor_widget = None
        asset_editor_row_container = None
        asset_editor_menu_bar = None
        script_canvas = None
        node_palette = None
        node_tree_view = None
        node_tree_search_frame = None
        node_tree_search_box = None

    def verify_added_params(self):
        for index in range(N_VAR_TYPES):
            if self.asset_editor_row_container.findChild(QtWidgets.QFrame, f"[{index}]") is None:
                return False
        return True

    @pyside_utils.wrap_async
    async def run_test(self):

        # Preconditions
        general.idle_enable(True)

        # 1) Open Asset Editor
        # Initially close the Asset Editor and then reopen to ensure we don't have any existing assets open
        general.close_pane(ASSET_EDITOR_UI) # this doesn't close a file that was previously open if you had just run a test that created an asset
        general.open_pane(ASSET_EDITOR_UI)
        helper.wait_for_condition(lambda: general.is_pane_visible(ASSET_EDITOR_UI), WAIT_TIME_3)

        # 2) Initially create new Script Event file with one method
        tools.initialize_asset_editor_qt_objects(self)
        action = pyside_utils.find_child_by_pattern(self.asset_editor_menu_bar, {"type": QtWidgets.QAction, "text": SCRIPT_EVENT_UI})
        action.trigger()
        result = helper.wait_for_condition(
            lambda: self.asset_editor_row_container.findChild(QtWidgets.QFrame, EVENTS_QT) is not None
                    and self.asset_editor_row_container.findChild(QtWidgets.QFrame, EVENTS_QT).findChild(QtWidgets.QToolButton, "") is not None,
            WAIT_TIME_3,
        )
        Report.result(Tests.new_event_created, result)

        # 3) Add new method and set name to it
        add_event = self.asset_editor_row_container.findChild(QtWidgets.QFrame, EVENTS_QT).findChild(QtWidgets.QToolButton, "")
        add_event.click()
        result = helper.wait_for_condition(
            lambda: self.asset_editor_widget.findChild(QtWidgets.QFrame, EVENT_NAME_QT) is not None, WAIT_TIME_3
        )
        Report.result(Tests.child_event_created, result)
        tools.expand_qt_container_rows(self, EVENT_NAME_QT)
        tools.expand_qt_container_rows(self, NAME_STRING)
        children = self.asset_editor_row_container.findChildren(QtWidgets.QFrame, NAME_STRING)
        for child in children:
            line_edit = child.findChild(QtWidgets.QLineEdit)
            if line_edit is not None and line_edit.text() == DEFAULT_METHOD_NAME:
                line_edit.setText(TEST_METHOD_NAME)

        # 4) Add new parameters of each type
        helper.wait_for_condition(lambda: self.asset_editor_row_container.findChild(QtWidgets.QFrame, PARAMETERS_QT) is not None, WAIT_TIME_3)
        parameters = self.asset_editor_row_container.findChild(QtWidgets.QFrame, PARAMETERS_QT)
        add_param = parameters.findChild(QtWidgets.QToolButton, "")
        for _ in range(N_VAR_TYPES):
            add_param.click()

        # 5) Verify if parameters are added
        result = helper.wait_for_condition(lambda: self.verify_added_params(), WAIT_TIME_3)
        Report.result(Tests.params_added, result)

        # 6) Expand the parameter rows (to render QFrame 'Type' for each param)
        for index in range(N_VAR_TYPES):
            tools.expand_qt_container_rows(self, f"[{index}]")

        # 7) Set different names and datatypes for each parameter
        tools.expand_qt_container_rows(self, NAME_STRING)
        children = self.asset_editor_row_container.findChildren(QtWidgets.QFrame, NAME_STRING)
        index = 0
        for child in children:
            line_edit = child.findChild(QtWidgets.QLineEdit)
            if line_edit is not None and line_edit.text() == "ParameterName":
                line_edit.setText(f"param_{index}")
                index += 1

        children = self.asset_editor_row_container.findChildren(QtWidgets.QFrame, "Type")
        index = 0
        for child in children:
            combo_box = child.findChild(QtWidgets.QComboBox)
            if combo_box is not None and index < N_VAR_TYPES:
                combo_box.setCurrentIndex(index)
                index += 1

        # 8) Save file and verify node in SC Node Palette
        Report.result(Tests.file_saved, tools.save_script_event_file(self, FILE_PATH))
        general.open_pane(SCRIPT_CANVAS_UI)
        helper.wait_for_condition(lambda: general.is_pane_visible(SCRIPT_CANVAS_UI), WAIT_TIME_3)
        tools.initialize_sc_qt_objects(self)
        tools.canvas_node_palette_search(self, TEST_METHOD_NAME, SEARCH_RETRY_ATTEMPTS)
        result = helper.wait_for_condition(lambda:
                                           pyside_utils.find_child_by_pattern(self.node_tree_view, {"text": TEST_METHOD_NAME}) is not None, WAIT_TIME_3)
        Report.result(Tests.node_found, result)

        # 9) Close Asset Editor
        general.close_pane(ASSET_EDITOR_UI)
        general.close_pane(SCRIPT_CANVAS_UI)


test = TestScriptEvents_AllParamDatatypes_CreationSuccess()
test.run_test()
