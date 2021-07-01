"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# Test case ID : C13895144
# Test Case Title : Run a level with multiple ragdolls and then switch levels



# fmt: off
class Tests():
    enter_game_mode_with_ragdolls    = ("Entered game mode With Ragdolls",    "Failed to enter game mode With Ragdolls")
    found_ragdolls                   = ("Found all ragdolls",                   "Did not find all ragdolls")
    exit_game_mode_with_ragdolls     = ("Exited game mode With Ragdolls",     "Couldn't exit game mode With Ragdolls")

    enter_game_mode_without_ragdolls = ("Entered game mode Without Ragdolls", "Failed to enter game mode Without Ragdolls")
    exit_game_mode_without_ragdolls  = ("Exited game mode Without Ragdolls",  "Couldn't exit game mode Without Ragdolls")
# fmt: on


def C13895144_Ragdoll_ChangeLevel():
    """
    Summary:
    Runs an automated test to ensure that switching from a level with many ragdolls to a level with no ragdolls does not
    crash the editor.

    Level Description:
        C13895144_Ragdoll_WithRagdoll:
        10 Ragdoll entities are placed in a row. Each Entity has an Actor, Anim Graph and PhysX Ragdoll component.

        C13895144_Ragdoll_NoRagdoll:
        The same default level configuration as previous, but with no ragdoll entities. Essentially an empty level.

    Expected Behavior:
    C13895144_Ragdoll_WithRagdoll loads, all entities are found, then C13895144_Ragdoll_NoRagdoll loads correctly.
    No crash should occur.

    Test Steps:
    1) Open level (with ragdolls)
    2) Enter game mode
    3) Find all of our ragdolls
    4) Exit game mode
    5) Open level (without ragdolls)
    6) Enter game mode
    7) Wait for no crash
    8) Exit game mode
    9) Close the editor

    Note:
    - This test file must be called from the Open 3D Engine Editor command terminal
    - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """
    import os
    import sys

    import ImportPathHelper as imports

    imports.init()

    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    import azlmbr.legacy.general as general

    WAIT = 3.0

    helper.init_idle()
    # 1) Open level
    helper.open_level("Physics", "C13895144_Ragdoll_WithRagdoll")

    # 2) Enter game mode
    helper.enter_game_mode(Tests.enter_game_mode_with_ragdolls)

    # 3) Find all of our ragdolls
    all_valid = True
    for id in range(0, 10):
        ragdoll_id = general.find_game_entity("Ragdoll_{}".format(id))
        if not ragdoll_id.IsValid():
            Report.info("Could not find Ragdoll_{}".format(id))
            all_valid = False

    Report.result(Tests.found_ragdolls, all_valid)

    # 4) Exit game mode
    helper.exit_game_mode(Tests.exit_game_mode_with_ragdolls)

    # 5) Open level (without ragdolls)
    helper.open_level("Physics", "C13895144_Ragdoll_NoRagdoll")

    # 6) Enter game mode
    helper.enter_game_mode(Tests.enter_game_mode_without_ragdolls)

    # 7) Wait for no crash
    general.idle_wait(WAIT)

    # 8) Exit game mode
    helper.exit_game_mode(Tests.exit_game_mode_without_ragdolls)


if __name__ == "__main__":
    import ImportPathHelper as imports
    imports.init()

    from editor_python_test_tools.utils import Report
    Report.start_test(C13895144_Ragdoll_ChangeLevel)
