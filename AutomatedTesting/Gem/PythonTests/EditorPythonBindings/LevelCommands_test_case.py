"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# Tests a portion of the Python API from CryEdit.cpp while the Editor is running

import os
import azlmbr.editor as editor
import azlmbr.bus as bus

# Try to create WaterSample level (return 1 means level with given name existed)
levelAlreadyExisted = 1
if (editor.EditorToolsApplicationRequestBus(bus.Broadcast, 'CreateLevelNoPrompt', 'WaterSample', 1024, False) is levelAlreadyExisted):
    print("WaterSample level has already been created")

# Open WaterSample level
if (editor.EditorToolsApplicationRequestBus(bus.Broadcast, 'OpenLevelNoPrompt', 'WaterSample') is True):
    print("WaterSample level opened")

# Get level path
levelpath = editor.EditorToolsApplicationRequestBus(bus.Broadcast, 'GetCurrentLevelPath')

# Split level path and get level name
path, filename = os.path.split(levelpath)

# Get level name
levelname = editor.EditorToolsApplicationRequestBus(bus.Broadcast, 'GetCurrentLevelName')

# Compare level name gotten from path to levelname
if (filename == levelname):
    print("Level name is correct")

# Remove Levels folder from path
parent, levels = os.path.split(path)

if (levels == "Levels"):
    print("The level is in the Levels folder")
    
# Get game folder
gamefolder = editor.EditorToolsApplicationRequestBus(bus.Broadcast, 'GetGameFolder')

# Compare game folder - normalize first because of the different formats
norm_gamefolder = os.path.normcase(gamefolder)
norm_parent = os.path.normcase(parent)

if (norm_parent == norm_gamefolder):
    print("Game folder is correct")

# Close editor
editor.EditorToolsApplicationRequestBus(bus.Broadcast, 'ExitNoPrompt')
print("Editor with WaterSample level opened closed")

