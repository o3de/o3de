"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
#
def check_result(result, msg):
    from editor_python_test_tools.utils import Report
    if not result:
        Report.result(msg, False)
        raise Exception(msg + " : FAILED")

def Editor_LevelCommands_Works():
    # Description: 
    # Tests a portion of the Python API from CryEdit.cpp while the Editor is running

    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper
    import azlmbr.bus as bus
    import azlmbr.editor as editor
    import azlmbr.legacy.general
    import os

    # Required for automated tests
    TestHelper.init_idle()

    # Open the test level
    TestHelper.open_level(directory="", level="Base")
    azlmbr.legacy.general.idle_wait_frames(1)

    # Get level path
    levelpath = editor.EditorToolsApplicationRequestBus(bus.Broadcast, 'GetCurrentLevelPath')

    # Split level path and get level name
    path, filename = os.path.split(levelpath)

    # Get level name
    levelname = editor.EditorToolsApplicationRequestBus(bus.Broadcast, 'GetCurrentLevelName')

    # Compare level name gotten from path to levelname
    check_result(filename == levelname, "Level name is correct")

    # Remove Levels folder from path
    parent, levels = os.path.split(path)

    check_result(levels == "Levels", "The level is in the Levels folder")
    
    # Get game folder
    gamefolder = editor.EditorToolsApplicationRequestBus(bus.Broadcast, 'GetGameFolder')

    # Compare game folder - normalize first because of the different formats
    norm_gamefolder = os.path.normcase(gamefolder)
    norm_parent = os.path.normcase(parent)

    check_result(norm_parent == norm_gamefolder, "Game folder is correct")

    # all tests worked
    Report.result("Editor_LevelCommands_Works ran", True)

if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Editor_LevelCommands_Works)




