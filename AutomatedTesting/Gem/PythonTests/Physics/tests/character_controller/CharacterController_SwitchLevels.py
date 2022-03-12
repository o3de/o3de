"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# Test case ID : C14654881
# Test Case Title : Switching levels from a level containing a character controller component
# should not lead to a crash



# fmt: off
class Tests:
    # level
    level1_enter_game_mode    = ("Entered game mode level 1",      "Failed to enter game mode level 1")
    level1_exit_game_mode     = ("Exited game mode level 1",       "Couldn't exit game mode level 1")
    CharacterController_found = ("Character controller was found", "Character controller was not found")
    level2_enter_game_mode    = ("Entered game mode level 2",      "Failed to enter game mode level 2")
    level2_exit_game_mode     = ("Exited game mode level 2",       "Couldn't exit game mode level 2")
# fmt: on


def CharacterController_SwitchLevels():
    """
    Summary:
    Runs an automated test to verify that switching levels from a level containing a character controller component
    does not lead to a crash

    Level Description:
    There are 2 levels used in this test:
    One contains an entity with a PhysX Character Controller component,
    the other one is empty.

    Expected Behavior:
    It should enter and then exit game mode without any errors in both levels.


    Test Steps:
    1.1) Load the level with PhysX Character Controller
    1.2) Enter game mode
    1.3) Find the entity with PhysX Character Controller component
    1.4) Exit game mode

    2.1) Load the empty level
    2.2) Enter game mode
    2.3) Exit game mode

    3) Close editor
    """

    import os
    import sys
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper
    import azlmbr.legacy.general as general
    import azlmbr.bus

    # Constants
    WAIT_FOR_ERRORS = 3.0

    helper.init_idle()

    # 1.1) Load level 1 (with character controller)
    helper.open_level("Physics", "CharacterController_SwitchLevels")

    # 1.2) Enter game mode
    helper.enter_game_mode(Tests.level1_enter_game_mode)

    # 1.3) Find and validate character controller entity
    characterController_id = general.find_game_entity("CharacterController")
    Report.critical_result(Tests.CharacterController_found, characterController_id.IsValid())

    # 1.4) Exit Game mode
    helper.exit_game_mode(Tests.level1_exit_game_mode)

    # 2.1) Load level 2 (empty level)
    helper.open_level("Physics", "Base")

    # 2.2) Enter game mode
    helper.enter_game_mode(Tests.level2_enter_game_mode)
    general.idle_wait(WAIT_FOR_ERRORS)

    # 2.3) Exit Game mode
    helper.exit_game_mode(Tests.level2_exit_game_mode)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(CharacterController_SwitchLevels)
