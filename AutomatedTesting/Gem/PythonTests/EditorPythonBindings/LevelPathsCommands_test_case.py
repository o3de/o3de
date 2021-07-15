"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# Tests a portion of the Python API from CryEdit.cpp while the Editor is running

import os
import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.legacy.general as general

# Open a level (any level should work)
editor.EditorToolsApplicationRequestBus(bus.Broadcast, 'OpenLevelNoPrompt', 'WaterSample')

# Get level path
levelpath = general.get_current_level_path()

# Split level path and get level name
path, level = os.path.split(levelpath)

# Get level name
levelname = general.get_current_level_name()

# Compare level name gotten from path to levelname
if(level == levelname):
    print("Level name is correct")

# Remove Levels folder from path
parent, levels = os.path.split(path)

if(levels == "Levels"):
    print("The level is in the Levels folder")
    
# Get game folder
gamefolder = general.get_game_folder()

# Compare game folder - normalize first because of the different formats
norm_gamefolder = os.path.normcase(gamefolder)
norm_parent = os.path.normcase(parent)

if(norm_parent == norm_gamefolder):
    print("Game folder is correct")

editor.EditorToolsApplicationRequestBus(bus.Broadcast, 'ExitNoPrompt')
