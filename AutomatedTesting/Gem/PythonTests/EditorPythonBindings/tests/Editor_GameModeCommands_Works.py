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

def Editor_GameModeCommands_Works():
    # Description: 
    # Tests the Python Game Mode API from PythonEditorFuncs.cpp while the Editor is running

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

    general.idle_enable(True)
    general.idle_wait(0.125)
    general.enter_game_mode()
    general.idle_wait(1.0)

    check_result(general.is_in_game_mode(), "Game Mode is On")

    general.idle_wait(0.125)
    general.exit_game_mode()
    general.idle_wait(1.0)

    check_result(not(general.is_in_game_mode()), "Game Mode is Off")

    # all tests worked
    Report.result("Editor_GameModeCommands_Works ran", True)

if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Editor_GameModeCommands_Works)







