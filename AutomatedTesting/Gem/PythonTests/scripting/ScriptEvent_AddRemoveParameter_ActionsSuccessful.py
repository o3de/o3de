"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# fmt: off
class Tests():
    new_event_created   = ("Successfully created a new event", "Failed to create a new event")
    child_event_created = ("Successfully created Child Event", "Failed to create Child Event")
    file_saved          = ("Successfully saved event asset",   "Failed to save event asset")
    parameter_created   = ("Successfully added parameter",     "Failed to add parameter")
    parameter_removed   = ("Successfully removed parameter",   "Failed to remove parameter")
# fmt: on


def ScriptEvent_AddRemoveParameter_ActionsSuccessful():
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
    import os
    from PySide2 import QtWidgets

    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper
    import editor_python_test_tools.pyside_utils as pyside_utils

    import azlmbr.bus as bus
    import azlmbr.editor as editor
    import azlmbr.legacy.general as general

    GENERAL_WAIT = 1.0  # seconds
    FILE_PATH = os.path.join("AutomatedTesting", "ScriptCanvas", "test_file.scriptevent")
    QtObject = object

    def create_script_event(asset_editor: QtObject, file_path: str) -> None:
        action = pyside_utils.find_child_by_pattern(menu_bar, {"type": QtWidgets.QAction, "text": "Script Events"})
        action.trigger()
        result = helper.wait_for_condition(
            lambda: container.findChild(QtWidgets.QFrame, "Events") is not None, 3 * GENERAL_WAIT
        )
        Report.result(Tests.new_event_created, result)

        # Add new child event
        add_event = container.findChild(QtWidgets.QFrame, "Events").findChild(QtWidgets.QToolButton, "")
        add_event.click()
        result = helper.wait_for_condition(
            lambda: asset_editor.findChild(QtWidgets.QFrame, "EventName") is not None, GENERAL_WAIT
        )
        Report.result(Tests.child_event_created, result)
        # Save the Script Event file
        editor.AssetEditorWidgetRequestsBus(bus.Broadcast, "SaveAssetAs", file_path)

        # Verify if file is created
        result = helper.wait_for_condition(lambda: os.path.exists(file_path), 3 * GENERAL_WAIT)
        Report.result(Tests.file_saved, result)

    def create_parameter(file_path: str) -> None:
        add_param = container.findChild(QtWidgets.QFrame, "Parameters").findChild(QtWidgets.QToolButton, "")
        add_param.click()
        result = helper.wait_for_condition(
            lambda: asset_editor_widget.findChild(QtWidgets.QFrame, "[0]") is not None, GENERAL_WAIT
        )
        Report.result(Tests.parameter_created, result)
        editor.AssetEditorWidgetRequestsBus(bus.Broadcast, "SaveAssetAs", file_path)

    def remove_parameter(file_path: str) -> None:
        remove_param = container.findChild(QtWidgets.QFrame, "[0]").findChild(QtWidgets.QToolButton, "")
        remove_param.click()
        result = helper.wait_for_condition(
            lambda: asset_editor_widget.findChild(QtWidgets.QFrame, "[0]") is None, GENERAL_WAIT
        )
        Report.result(Tests.parameter_removed, result)
        editor.AssetEditorWidgetRequestsBus(bus.Broadcast, "SaveAssetAs", file_path)

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
    create_script_event(asset_editor_widget, FILE_PATH)

    # 4) Add Parameter to Event
    create_parameter(FILE_PATH)

    # 5) Remove Parameter from Event
    remove_parameter(FILE_PATH)


if __name__ == "__main__":
    import ImportPathHelper as imports

    imports.init()
    from editor_python_test_tools.utils import Report

    Report.start_test(ScriptEvent_AddRemoveParameter_ActionsSuccessful)
