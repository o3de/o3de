"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


def Menus_ViewMenuOptions_Work():
    """
    Summary:
    Interact with View Menu options and verify if all the options are working.

    Expected Behavior:
    The View menu functions normally.

    Test Steps:
     1) Open an existing level
     2) Interact with View Menu options

    Note:
    - This test file must be called from the O3DE Editor command terminal
    - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    import editor_python_test_tools.pyside_utils as pyside_utils
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    view_menu_options = [
        ("Center on Selection",),
        ("Show Quick Access Bar",),
        ("Viewport", "Configure Layout"),
        ("Viewport", "Go to Position"),
        ("Viewport", "Center on Selection"),
        ("Viewport", "Go to Location"),
        ("Viewport", "Remember Location"),
        ("Viewport", "Switch Camera"),
        ("Viewport", "Show/Hide Helpers"),
        ("Refresh Style",),
    ]

    # 1) Open an existing simple level
    helper.init_idle()
    helper.open_level("Physics", "Base")

    # 2) Interact with View Menu options
    editor_window = pyside_utils.get_editor_main_window()
    for option in view_menu_options:
        try:
            action = pyside_utils.get_action_for_menu_path(editor_window, "View", *option)
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
    Report.start_test(Menus_ViewMenuOptions_Work)
