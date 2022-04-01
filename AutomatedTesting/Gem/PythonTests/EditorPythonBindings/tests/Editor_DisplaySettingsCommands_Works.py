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

def Editor_DisplaySettingsCommands_Works():
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
    TestHelper.open_level(directory="", level="TestDependenciesLevel")
    azlmbr.legacy.general.idle_wait_frames(1)

    # Retrieve current settings
    existingSettings = settings.get_misc_editor_settings()

    # Alter settings
    alteredSettings = existingSettings + 12

    # Set altered settings
    settings.set_misc_editor_settings(alteredSettings)

    # Get settings again
    newSettings = settings.get_misc_editor_settings()

    # Check if the setter worked
    if(alteredSettings == newSettings):
        print("display settings were changed correctly")

    # Restore previous settings
    settings.set_misc_editor_settings(existingSettings)

    # all tests worked
    Report.result("Editor_DisplaySettingsCommands_Works ran", True)

if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Editor_DisplaySettingsCommands_Works)

