"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

# Tests the Python API from DisplaySettingsPythonFuncs.cpp while the Editor is running

import azlmbr.bus as bus
import azlmbr.editor as editor
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
if(alteredSettings == newSettings):
    print("display settings were changed correctly")

# Restore previous settings
settings.set_misc_editor_settings(existingSettings)

editor.EditorToolsApplicationRequestBus(bus.Broadcast, 'ExitNoPrompt')
