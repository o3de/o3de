"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


def Menus_FileMenuOptions_Work():
    """
    Summary:
    Interact with File Menu options and verify if all the options are working.

    Expected Behavior:
    The File menu functions normally.

    Test Steps:
     1) Open level
     2) Interact with File Menu options

    Note:
    - This test file must be called from the O3DE Editor command terminal
    - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    import azlmbr.legacy.general as general

    import editor_python_test_tools.hydra_editor_utils as hydra
    import pyside_utils
    from editor_python_test_tools.utils import Report

    file_menu_options = [
        ("New Level",),
        ("Open Level",),
        ("Open Recent",),
        ("Save",),
        ("Save As",),
        ("Save Level Statistics",),
        ("Edit Project Settings",),
        ("Edit Platform Settings",),
        ("New Project",),
        ("Open Project",),
        # Disabling "Show Log File" for CI testing as it launches an external app. Uncomment as needed for local testing
        # ("Show Log File",),
    ]

    # 1) Open an existing simple level
    hydra.open_base_level()

    # The action manager doesn't register the menus until the next system tick, so need to wait
    # until the menu bar has been populated
    general.idle_enable(True)
    general.idle_wait_frames(1)

    # 2) Interact with File Menu options
    editor_window = pyside_utils.get_editor_main_window()
    for option in file_menu_options:
        try:
            action = pyside_utils.get_action_for_menu_path(editor_window, "File", *option)
            Report.info(f"Triggering {action.iconText()}")
            action.trigger()
            action_triggered = True
        except Exception as e:
            action_triggered = False
            print(e)
        menu_action_triggered = (
            f"{action.iconText()} action triggered successfully",
            f"Failed to trigger {action.iconText()} action"
        )
        Report.result(menu_action_triggered, action_triggered)


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(Menus_FileMenuOptions_Work)
