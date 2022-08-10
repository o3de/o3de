"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
from PySide2 import QtWidgets
from editor_python_test_tools.utils import Report
import pyside_utils
import scripting_utils.scripting_tools as tools
import azlmbr.legacy.general as general
from scripting_utils.scripting_constants import (ASSET_EDITOR_UI, SCRIPT_EVENT_FILE_PATH)

# fmt: off
class Tests():
    file_saved = ("Successfully saved event asset", "Failed to save event asset")
    asset_editor_opened = ("asset editor successfully opened",    "Asset editor failed to open")
# fmt: on

class ScriptEvent_AddRemoveParameter_ActionsSuccessful:
    """
    Summary:
     Parameter can be removed from a Script Event method

    Expected Behavior:
     Upon saving the updated .scriptevents asset the removed paramenter should no longer be present on the Script Event

    Test Steps:
     1) Open Asset Editor
     2) Initialize the editor and asset editor qt objects
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
        self.editor_main_window = None
        self.asset_editor = None
        self.asset_editor_widget = None
        self.asset_editor_row_container = None
        self.asset_editor_menu_bar = None

    @pyside_utils.wrap_async
    async def run_test(self):

        # Preconditions
        general.idle_enable(True)
        general.close_pane(ASSET_EDITOR_UI)

        # 1) Open the asset editor
        result = tools.open_asset_editor()
        Report.result(Tests.asset_editor_opened, result)

        # 2) Initialize the editor and asset editor qt objects
        tools.initialize_editor_object(self)
        tools.initialize_asset_editor_object(self)

        # 3) Create new Script Event Asset
        tools.create_script_event(self)
        result = tools.save_script_event_file(self, SCRIPT_EVENT_FILE_PATH)
        Report.result(Tests.file_saved, result)

        # 4) Add Parameter to Event
        tools.create_script_event_parameter(self)
        result = tools.save_script_event_file(self, SCRIPT_EVENT_FILE_PATH)
        Report.result(Tests.file_saved, result)

        # 5) Remove Parameter from Event
        tools.remove_script_event_parameter(self)
        result = tools.save_script_event_file(self, SCRIPT_EVENT_FILE_PATH)
        Report.result(Tests.file_saved, result)


test = ScriptEvent_AddRemoveParameter_ActionsSuccessful()
test.run_test()
