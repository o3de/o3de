"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import os, sys
sys.path.append(os.path.dirname(__file__))
from Editor_TestClass import BaseClass

class Editor_GameModeCommands_Works(BaseClass):
    # Description: 
    # Tests the Python Game Mode API from PythonEditorFuncs.cpp while the Editor is running
    
    @staticmethod
    def test():
        import azlmbr.bus as bus
        import azlmbr.editor as editor
        import azlmbr.legacy.general as general

        general.idle_enable(True)
        general.idle_wait(0.125)
        general.enter_game_mode()
        general.idle_wait(1.0)

        BaseClass.check_result(general.is_in_game_mode(), "Game Mode is On")

        general.idle_wait(0.125)
        general.exit_game_mode()
        general.idle_wait(1.0)

        BaseClass.check_result(not(general.is_in_game_mode()), "Game Mode is Off")

if __name__ == "__main__":
    tester = Editor_GameModeCommands_Works()
    tester.test_case(tester.test)

