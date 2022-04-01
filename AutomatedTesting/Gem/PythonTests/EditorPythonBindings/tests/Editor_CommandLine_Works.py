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

def Editor_CommandLine_Works():
    # Description: 
    # Tests the Python API from DisplaySettingsPythonFuncs.cpp while the Editor is running

    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper
    import azlmbr.bus as bus
    import azlmbr.editor as editor
    import azlmbr.legacy.general
    import azlmbr.entity as entity
    import azlmbr.legacy.settings as settings

    # Required for automated tests
    TestHelper.init_idle()

    # Open the test level
    TestHelper.open_level(directory="", level="Base")
    azlmbr.legacy.general.idle_wait_frames(1)

    # make sure the @engroot@ exists as a azlmbr.paths property
    engroot = azlmbr.paths.engroot
    check_result(engroot is not None and len(engroot) is not 0, f'engroot is {engroot}')

    # resolving a basic path
    path = azlmbr.paths.resolve_path('@engroot@/engineassets/icons/averagememoryusage.tif.streamingimage')
    check_result(len(path) != 0 and path.find('@engroot@') == -1, f'path resolved to {path}')

    # all tests worked
    Report.result("Editor_CommandLine_Works ran", True)

if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Editor_CommandLine_Works)

