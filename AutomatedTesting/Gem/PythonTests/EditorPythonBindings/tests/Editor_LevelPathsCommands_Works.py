"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import os, sys
sys.path.append(os.path.dirname(__file__))
from Editor_TestClass import BaseClass

class Editor_LevelPathsCommands_Works(BaseClass):
    # Description: 
    # Tests a portion of the Python API from CryEdit.cpp while the Editor is running
    
    @staticmethod
    def test():
        import azlmbr.bus as bus
        import azlmbr.editor as editor
        import azlmbr.legacy.general as general

        # Get level path
        levelpath = general.get_current_level_path()

        # Split level path and get level name
        path, level = os.path.split(levelpath)

        # Get level name
        levelname = general.get_current_level_name()

        # Compare level name gotten from path to levelname
        BaseClass.check_result(level == levelname, "Level name is correct")

        # Remove Levels folder from path
        parent, levels = os.path.split(path)
        BaseClass.check_result(levels == "Levels", "The level is in the Levels folder")
    
        # Get game folder
        gamefolder = general.get_game_folder()

        # Compare game folder - normalize first because of the different formats
        norm_gamefolder = os.path.normcase(gamefolder)
        norm_parent = os.path.normcase(parent)

        BaseClass.check_result(norm_parent == norm_gamefolder, "Game folder is correct")

if __name__ == "__main__":
    tester = Editor_LevelPathsCommands_Works()
    tester.test_case(tester.test)

