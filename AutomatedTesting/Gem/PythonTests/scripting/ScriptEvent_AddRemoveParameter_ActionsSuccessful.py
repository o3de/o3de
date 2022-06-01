"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
from PySide2 import QtWidgets
from editor_python_test_tools.utils import Report
from editor_python_test_tools.utils import TestHelper as helper
import editor_python_test_tools.pyside_utils as pyside_utils
import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.paths as paths
import azlmbr.legacy.general as general
from scripting_utils.scripting_constants import (ASSET_EDITOR_UI, NODE_PALETTE_UI, SCRIPT_EVENT_UI,
                                                 NODE_PALETTE_QT, EVENTS_QT, NODE_TEST_METHOD, DEFAULT_SCRIPT_EVENT,
                                                 TREE_VIEW_QT, DEFAULT_METHOD_NAME, SAVE_STRING, WAIT_TIME_3)
# fmt: off
class Tests():
    new_event_created   = ("Successfully created a new event", "Failed to create a new event")
    child_event_created = ("Successfully created Child Event", "Failed to create Child Event")
    file_saved          = ("Successfully saved event asset",   "Failed to save event asset")
    parameter_created   = ("Successfully added parameter",     "Failed to add parameter")
    parameter_removed   = ("Successfully removed parameter",   "Failed to remove parameter")
# fmt: on

FILE_PATH = os.path.join(paths.projectroot, "ScriptCanvas", "test_file.scriptevent")

class ScriptEvent_AddRemoveParameter_ActionsSuccessful:
    """
    Summary:
     Parameter can be removed from a Script Event method

    Expected Behavior:
     Upon saving the updated .scriptevents asset the removed paramenter should no longer be present on the Script Event

    Test Steps:
     1) Open Asset Editor
     2) Get Asset Editor Qt object
     3) Create new Script Event Asset
     4) Add Parameter to Event
     5) Remove Parameter from Event

    Note:
     - This test file must be called from the Open 3D Engine Editor command terminal
     - Any passed and failed tests are written to the Editor.log file.
        Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    def __init__(self):
        self.editor_window = None
        self.asset_editor = None
        self.asset_editor_widget = None
        self.asset_editor_row_container = None
        self.asset_editor_pulldown_menu = None

    def create_script_event(self) -> None:
        action = pyside_utils.find_child_by_pattern(self.asset_editor_pulldown_menu, {"type": QtWidgets.QAction, "text": SCRIPT_EVENT_UI})
        action.trigger()
        result = helper.wait_for_condition(
            lambda: self.asset_editor_row_container.findChild(QtWidgets.QFrame, EVENTS_QT) is not None, WAIT_TIME_3
        )
        Report.result(Tests.new_event_created, result)

        # Add new child event
        add_event = self.asset_editor_row_container.findChild(QtWidgets.QFrame, EVENTS_QT).findChild(QtWidgets.QToolButton, "")
        add_event.click()
        result = helper.wait_for_condition(
            lambda: self.asset_editor_widget.findChild(QtWidgets.QFrame, DEFAULT_SCRIPT_EVENT) is not None, WAIT_TIME_3
        )
        Report.result(Tests.child_event_created, result)
        # Save the Script Event file
        editor.AssetEditorWidgetRequestsBus(bus.Broadcast, "SaveAssetAs", FILE_PATH)

        # Verify if file is created
        result = helper.wait_for_condition(lambda: os.path.exists(FILE_PATH), WAIT_TIME_3)
        Report.result(Tests.file_saved, result)

    def create_parameter(self) -> None:
        add_param = self.asset_editor_row_container.findChild(QtWidgets.QFrame, "Parameters").findChild(QtWidgets.QToolButton, "")
        add_param.click()
        result = helper.wait_for_condition(
            lambda: self.asset_editor_widget.findChild(QtWidgets.QFrame, "[0]") is not None, WAIT_TIME_3
        )
        Report.result(Tests.parameter_created, result)
        editor.AssetEditorWidgetRequestsBus(bus.Broadcast, "SaveAssetAs", FILE_PATH)

    def remove_parameter(self) -> None:
        remove_param = self.asset_editor_row_container.findChild(QtWidgets.QFrame, "[0]").findChild(QtWidgets.QToolButton, "")
        remove_param.click()
        result = helper.wait_for_condition(
            lambda: self.asset_editor_widget.findChild(QtWidgets.QFrame, "[0]") is None, WAIT_TIME_3
        )
        Report.result(Tests.parameter_removed, result)
        editor.AssetEditorWidgetRequestsBus(bus.Broadcast, "SaveAssetAs", FILE_PATH)

    @pyside_utils.wrap_async
    async def run_test(self):

        # Preconditions
        general.idle_enable(True)

        # 1) Open Asset Editor
        # Initially close the Asset Editor and then reopen to ensure we don't have any existing assets open
        general.close_pane(ASSET_EDITOR_UI)
        general.open_pane(ASSET_EDITOR_UI)
        helper.wait_for_condition(lambda: general.is_pane_visible(ASSET_EDITOR_UI), WAIT_TIME_3)

        # 2) Get Asset Editor Qt object
        self.editor_window = pyside_utils.get_editor_main_window()
        self.asset_editor = self.editor_window.findChild(QtWidgets.QDockWidget, ASSET_EDITOR_UI)
        self.asset_editor_widget = self.asset_editor.findChild(QtWidgets.QWidget, "AssetEditorWindowClass")
        self.asset_editor_row_container = self.asset_editor_widget.findChild(QtWidgets.QWidget, "ContainerForRows")
        self.asset_editor_pulldown_menu = self.asset_editor_widget.findChild(QtWidgets.QMenuBar)

        # 3) Create new Script Event Asset
        self.create_script_event()

        # 4) Add Parameter to Event
        self.create_parameter()

        # 5) Remove Parameter from Event
        self.remove_parameter()


test = ScriptEvent_AddRemoveParameter_ActionsSuccessful()
test.run_test()
