"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import os, sys
sys.path.append(os.path.dirname(__file__))
from Editor_TestClass import BaseClass

class Editor_DisplaySettingsCommands_Works(BaseClass):
    # Description: 
    # Tests the Python API from DisplaySettingsPythonFuncs.cpp while the Editor is running
    
    @staticmethod
    def test():
        import azlmbr.legacy.settings as settings

        # Retrieve current settings
        existingSettings = settings.get_misc_editor_settings()

        # Alter settings
        alteredSettings = existingSettings + 12

        # Set altered settings
        settings.set_misc_editor_settings(alteredSettings)

        # Get settings again
        newSettings = settings.get_misc_editor_settings()

        # Check if the setter worked
        BaseClass.check_result(alteredSettings == newSettings, 'display settings were changed correctly')

        # Restore previous settings
        settings.set_misc_editor_settings(existingSettings)

if __name__ == "__main__":
    tester = Editor_DisplaySettingsCommands_Works()
    tester.test_case(tester.test)

