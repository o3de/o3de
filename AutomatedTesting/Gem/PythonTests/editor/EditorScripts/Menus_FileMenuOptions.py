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

    import editor_python_test_tools.pyside_utils as pyside_utils
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    file_menu_options = [
        ("New Level",),
        ("Open Level",),
        ("Import",),
        ("Save",),
        ("Save As",),
        ("Save Level Statistics",),
        ("Edit Project Settings",),
        ("Edit Platform Settings",),
        ("New Project",),
        ("Open Project",),
        ("Show Log File",),
        ("Resave All Slices",),
        ("Exit",),
    ]

    # 1) Open an existing simple level
    helper.init_idle()
    helper.open_level("Physics", "Base")

    # 2) Interact with File Menu options
    editor_window = pyside_utils.get_editor_main_window()
    for option in file_menu_options:
        try:
            action = pyside_utils.get_action_for_menu_path(editor_window, "File", *option)
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
