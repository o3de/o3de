"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# fmt: off
class Tests():
    new_event_created   = ("New Script Event created",         "New Script Event not created")
    child_event_created = ("Child Event created",              "Child not created")
    file_saved          = ("Script event file saved",          "Script event file did not save")
    console_error       = ("No unexpected error in console",   "Error found in console")
    console_warning     = ("No unexpected warning in console", "Warning found in console")
# fmt: on


def CreateScriptEventFile():
    """
    Summary:
     Script Event file can be created

    Expected Behavior:
     File is created without any errors and warnings in Console

    Test Steps:
     1) Open Asset Editor
     2) Get Asset Editor Qt object
     3) Create new Script Event Asset
     4) Add new child event
     5) Save the Script Event file
     6) Verify if file is created
     7) Verify console for errors/warnings
     8) Close Asset Editor

    Note:
     - This test file must be called from the Open 3D Engine Editor command terminal
     - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """
    import os
    from utils import Report
    from utils import TestHelper as helper
    from utils import Tracer
    import pyside_utils

    # Open 3D Engine imports
    import azlmbr.legacy.general as general
    import azlmbr.editor as editor
    import azlmbr.bus as bus

    # Pyside imports
    from PySide2 import QtWidgets

    GENERAL_WAIT = 1.0  # seconds

    FILE_PATH = os.path.join("AutomatedTesting", "ScriptCanvas", "test_file.scriptevent")

    # 1) Open Asset Editor
    general.idle_enable(True)
    # Initially close the Asset Editor and then reopen to ensure we don't have any existing assets open
    general.close_pane("Asset Editor")
    general.open_pane("Asset Editor")
    helper.wait_for_condition(lambda: general.is_pane_visible("Asset Editor"), 5.0)

    # 2) Get Asset Editor Qt object
    editor_window = pyside_utils.get_editor_main_window()
    asset_editor_widget = editor_window.findChild(QtWidgets.QDockWidget, "Asset Editor").findChild(
        QtWidgets.QWidget, "AssetEditorWindowClass"
    )
    container = asset_editor_widget.findChild(QtWidgets.QWidget, "ContainerForRows")
    menu_bar = asset_editor_widget.findChild(QtWidgets.QMenuBar)

    # 3) Create new Script Event Asset
    action = pyside_utils.find_child_by_pattern(menu_bar, {"type": QtWidgets.QAction, "text": "Script Events"})
    action.trigger()
    result = helper.wait_for_condition(
        lambda: container.findChild(QtWidgets.QFrame, "Events") is not None, 3 * GENERAL_WAIT
    )
    Report.result(Tests.new_event_created, result)

    # 4) Add new child event
    add_event = container.findChild(QtWidgets.QFrame, "Events").findChild(QtWidgets.QToolButton, "")
    add_event.click()
    result = helper.wait_for_condition(
        lambda: asset_editor_widget.findChild(QtWidgets.QFrame, "EventName") is not None, GENERAL_WAIT
    )
    Report.result(Tests.child_event_created, result)

    with Tracer() as section_tracer:
        # 5) Save the Script Event file
        editor.AssetEditorWidgetRequestsBus(bus.Broadcast, "SaveAssetAs", FILE_PATH)

        # 6) Verify if file is created
        result = helper.wait_for_condition(lambda: os.path.exists(FILE_PATH), 3 * GENERAL_WAIT)
        Report.result(Tests.file_saved, result)

    # 7) Verify console for errors/warnings
    Report.result(Tests.console_error, not section_tracer.has_errors)
    Report.result(Tests.console_warning, not section_tracer.has_warnings)

    # 8) Close Asset Editor
    general.close_pane("Asset Editor")


if __name__ == "__main__":
    import ImportPathHelper as imports

    imports.init()
    from utils import Report

    Report.start_test(CreateScriptEventFile)
