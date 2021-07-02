"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
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
