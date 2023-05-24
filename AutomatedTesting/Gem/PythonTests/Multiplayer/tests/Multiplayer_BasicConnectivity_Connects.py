"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# Test Case Title : Check that the basic client connect test works


# fmt: off
class TestConstants:
    enter_game_mode =           ("Entered game mode",                       "Failed to enter game mode")
    exit_game_mode =            ("Exited game mode",                        "Couldn't exit game mode")
    find_network_player =       ("Found network player",                    "Couldn't find network player")
# fmt: on


def Multiplayer_BasicConnectivity_Connects():
    r"""
    Summary:
    Runs a test to make sure that a networked player can be spawned

    Level Description:
    - Dynamic
        1. Although the level is empty, when the server and editor connect the server will spawn the player prefab.
    - Static
        1. This is an empty level.
                    
    Expected Outcome:
    We should see the player connect and spawn with no errors.
    
    :return:
    """
    import azlmbr.legacy.general as general
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper


    level_name = "BasicConnectivity_Connects"
    helper.init_idle()

    # 1) Open Level
    helper.open_level("Multiplayer", level_name)
    general.set_cvar_integer('editorsv_port', 33454)

    # 2) Enter game mode
    helper.multiplayer_enter_game_mode(TestConstants.enter_game_mode, helper.EditorServerMode.DEDICATED_SERVER)

    # 3) Make sure the network player was spawned
    player_id = general.find_game_entity("Player")
    Report.critical_result(TestConstants.find_network_player, player_id.IsValid())

    # Exit game mode
    helper.exit_game_mode(TestConstants.exit_game_mode)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Multiplayer_BasicConnectivity_Connects)
    
