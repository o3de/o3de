"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
def Editor_ComponentCommands_BuildComponentTypeNameList():
    # Description: 
    # LY-107818 reported a crash with this code snippet
    # This new test will be used to regress test the issue

    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper
    import azlmbr.bus as bus
    import azlmbr.editor as editor
    import azlmbr.legacy.general

    # Required for automated tests
    TestHelper.init_idle()

    # Open the test level
    TestHelper.open_level(directory="", level="Base")
    azlmbr.legacy.general.idle_wait_frames(1)

    try:
        componentList = editor.EditorComponentAPIBus(bus.Broadcast, 'BuildComponentTypeNameList')
        if(componentList is not None and len(componentList) > 0):
            Report.result("BuildComponentTypeNameList returned a valid list", True)

    except:
        Report.result("BuildComponentTypeNameList usage threw an exception", False)


    # all tests worked
    Report.result("BuildComponentTypeNameList ran", True)

if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Editor_ComponentCommands_BuildComponentTypeNameList)
