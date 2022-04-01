"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
def check_result(result, msg):
    from editor_python_test_tools.utils import Report
    if not result:
        Report.result(msg, False)
        raise Exception(msg + " : FAILED")

def Editor_MainWindowCommands_Works():
    # Description: 
    # Tests the Python API from MainWindow.cpp while the Editor is running

    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper
    import azlmbr.bus as bus
    import azlmbr.editor as editor
    import azlmbr.legacy.general as general

    # Required for automated tests
    TestHelper.init_idle()

    # Open the test level
    TestHelper.open_level(directory="", level="Base")
    azlmbr.legacy.general.idle_wait_frames(1)

    # Get all pane names
    panes = general.get_pane_class_names()

    check_result(len(panes) > 0, f'get_pane_class_names worked of {panes}')

    # Get any element from the panes list
    test_pane = panes[0]

    general.open_pane(test_pane)

    check_result(general.is_pane_visible(test_pane), f'open_pane worked with {test_pane}')

    general.close_pane(test_pane)

    check_result(not(general.is_pane_visible(test_pane)), f'close_pane worked with {test_pane}')

    # all tests worked
    Report.result("Editor_MainWindowCommands_Works ran", True)

if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Editor_MainWindowCommands_Works)

