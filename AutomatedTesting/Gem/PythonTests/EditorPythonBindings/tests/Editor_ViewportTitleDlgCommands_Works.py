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

def Editor_ViewportTitleDlgCommands_Works():
    # Description: 
    # Tests the ViewportTitleDlg Python API from ViewportTitleDlg.cpp while the Editor is running

    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper
    import azlmbr.math
    import azlmbr.legacy.general as general

    # Required for automated tests
    TestHelper.init_idle()

    # Open the test level
    TestHelper.open_level(directory="", level="Base")
    azlmbr.legacy.general.idle_wait_frames(1)

    # Get the current state of our helpers toggle.  
    initial_state = general.is_helpers_shown()

    # Toggle it once and verify it changed
    general.toggle_helpers()
    check_result(initial_state != general.is_helpers_shown(), "Toggle it once and verify it changed")

    # Toggle it a second time and verify it changed back to the original setting.
    # Note:  We specifically choose an even number of times so we leave the Editor
    # in the same state as we found it. :)
    general.toggle_helpers()
    check_result(initial_state == general.is_helpers_shown(), "is_helpers_shown + toggle_helpers")

    # all tests worked
    Report.result("Editor_ViewportTitleDlgCommands_Works ran", True)

if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Editor_ViewportTitleDlgCommands_Works)


