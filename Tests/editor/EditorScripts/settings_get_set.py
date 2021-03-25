"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

"""
Test the Hydra API to access Editor Settings
"""

import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.math as math
from Tests.editor.editor_utils.editor_test_helper import EditorTestHelper

class TestSettingsAPI(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="settings_get_set: ", args=["level"])

    def run_test(self):
        """
        Summary:
        Tests the Hydra API for Settings.

        Expected Behavior:
        Settings can be listed, accessed and set.

        Test Steps:
        1) Build the list of all settings
        2) For each supported type (bool, int, float, string):
            2a) Get the current values
            2b) Set an altered values
            2c) Get the value again
            2d) Verify the value has changed
            2e) Restore original value

        Note:
        - This test file must be called from the Lumberyard Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """
        
        # Get full list of Editor settings
        paths = editor.EditorSettingsAPIBus(bus.Broadcast, 'BuildSettingsList')

        if(len(paths) > 0):
            print("BuildSettingsList returned a non-empty list")


        # Verify a boolean settings
        def ParseBoolValue(value):
            if(value == "0"):
                return False
            return True

        boolOutcome = editor.EditorSettingsAPIBus(bus.Broadcast, 'GetValue', 'Settings|LoadLastLevelAtStartup')

        if(boolOutcome.isSuccess()):
             startupValue = boolOutcome.GetValue()

        editor.EditorSettingsAPIBus(bus.Broadcast, 'SetValue', 'Settings|LoadLastLevelAtStartup', not(ParseBoolValue(startupValue)))

        boolOutcome = editor.EditorSettingsAPIBus(bus.Broadcast, 'GetValue', 'Settings|LoadLastLevelAtStartup')

        if(boolOutcome.isSuccess()):
             newStartupValue = boolOutcome.GetValue()

        if not(ParseBoolValue(startupValue) == ParseBoolValue(newStartupValue)):
            print("Boolean Setting editing works")

        # Restore previous value
        editor.EditorSettingsAPIBus(bus.Broadcast, 'SetValue', 'Settings|LoadLastLevelAtStartup', ParseBoolValue(startupValue))


        # Verify an int settings
        intOutcome = editor.EditorSettingsAPIBus(bus.Broadcast, 'GetValue', 'Settings|UndoLevels')

        if(intOutcome.isSuccess()):
             undoLevel = intOutcome.GetValue()

        editor.EditorSettingsAPIBus(bus.Broadcast, 'SetValue', 'Settings|UndoLevels', (int(undoLevel) + 10))

        intOutcome = editor.EditorSettingsAPIBus(bus.Broadcast, 'GetValue', 'Settings|UndoLevels')

        if(intOutcome.isSuccess()):
             newUndoLevel = intOutcome.GetValue()

        if not(int(undoLevel) == int(newUndoLevel)):
            print("Int Setting editing works")

        # Restore previous value
        editor.EditorSettingsAPIBus(bus.Broadcast, 'SetValue', 'Settings|UndoLevels', int(undoLevel))


        # Verify a float settings
        floatOutcome = editor.EditorSettingsAPIBus(bus.Broadcast, 'GetValue', 'Settings|DeepSelectionNearness')

        if(floatOutcome.isSuccess()):
             deepSelection = floatOutcome.GetValue()

        editor.EditorSettingsAPIBus(bus.Broadcast, 'SetValue', 'Settings|DeepSelectionNearness', (float(deepSelection) + 0.5))

        floatOutcome = editor.EditorSettingsAPIBus(bus.Broadcast, 'GetValue', 'Settings|DeepSelectionNearness')

        if(floatOutcome.isSuccess()):
             newDeepSelection = floatOutcome.GetValue()

        if not(float(deepSelection) == float(newDeepSelection)):
            print("Float Setting editing works")

        # Restore previous value
        editor.EditorSettingsAPIBus(bus.Broadcast, 'SetValue', 'Settings|DeepSelectionNearness', float(deepSelection))


        # Verify a string settings
        stringOutcome = editor.EditorSettingsAPIBus(bus.Broadcast, 'GetValue', 'Settings|TemporaryDirectory')

        if(stringOutcome.isSuccess()):
             tempDir = stringOutcome.GetValue()

        editor.EditorSettingsAPIBus(bus.Broadcast, 'SetValue', 'Settings|TemporaryDirectory', "SomeTempDirectory")

        stringOutcome = editor.EditorSettingsAPIBus(bus.Broadcast, 'GetValue', 'Settings|TemporaryDirectory')

        if(stringOutcome.isSuccess()):
             newTempDir = stringOutcome.GetValue()

        if not(tempDir == newTempDir):
            print("String Setting editing works")

        # Restore previous value
        editor.EditorSettingsAPIBus(bus.Broadcast, 'SetValue', 'Settings|TemporaryDirectory', tempDir)


test = TestSettingsAPI()
test.run()