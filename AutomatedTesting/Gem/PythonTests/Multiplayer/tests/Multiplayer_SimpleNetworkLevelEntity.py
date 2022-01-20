"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# Test Case Title : Check that level entities with network bindings are properly replicated.
# Note: This test should be ran on a fresh editor run; some bugs with spawnables occur only on the first editor play-mode.


# fmt: off
class TestSuccessFailTuples():
    enter_game_mode =           ("Entered game mode",                       "Failed to enter game mode")
    exit_game_mode =            ("Exited game mode",                        "Couldn't exit game mode")
    find_network_player =       ("Found network player",                    "Couldn't find network player")
# fmt: on


def Multiplayer_SimpleNetworkLevelEntity():
    r"""
    Summary:
    Test to make sure that network entities in a level function and are replicated to clients as expected

    Level Description:
    - Static
        1. NetLevelEntity. This is a networked entity which has a script attached which prints logs to ensure it's replicated.
    
                    
    Expected Outcome:
    We should see logs stating that the net-sync'd level entity exists on both server and client.

    :return:
    """
    import azlmbr.legacy.general as general
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import Tracer

    from editor_python_test_tools.utils import TestHelper as helper
    from ly_remote_console.remote_console_commands import RemoteConsole as RemoteConsole

    level_name = "SimpleNetworkLevelEntity"
    player_prefab_name = "Player"
    player_prefab_path = f"levels/multiplayer/{level_name}/{player_prefab_name}.network.spawnable"

    helper.init_idle()

    # 1) Open Level
    helper.open_level("Multiplayer", level_name)

    with Tracer() as section_tracer:
        # 2) Enter game mode
        helper.multiplayer_enter_game_mode(TestSuccessFailTuples.enter_game_mode, player_prefab_path.lower())

        # 3) Make sure the network player was spawned
        player_id = general.find_game_entity(player_prefab_name)
        Report.critical_result(TestSuccessFailTuples.find_network_player, player_id.IsValid())

        # 4) Check the editor logs for network spawnable errors
        ATTEMPTING_INVALID_NETSPAWN_WAIT_TIME_SECONDS = 1.0  # The editor will try to net-spawn its networked level entity before it's even a client. Make sure this doesn't happen.
        helper.fail_if_log_line_found('NetworkEntityManager', "RequestNetSpawnableInstantiation: Requested spawnable Root.network.spawnable doesn't exist in the NetworkSpawnableLibrary. Please make sure it is a network spawnable", section_tracer.errors, ATTEMPTING_INVALID_NETSPAWN_WAIT_TIME_SECONDS)

        # 5) Ensure the script graph attached to the level entity is running on both client and server
        SCRIPTGRAPH_ENABLED_WAIT_TIME_SECONDS = 0.25
        # Check Server
        helper.succeed_if_log_line_found('EditorServer', "Script: SimpleNetworkLevelEntity: On Graph Start", section_tracer.prints, SCRIPTGRAPH_ENABLED_WAIT_TIME_SECONDS)
        # Check Editor/Client (Uncomment once script asset preload is working properly LYN-9136)
        # helper.succeed_if_log_line_found('Script', "SimpleNetworkLevelEntity: On Graph Start", section_tracer.prints, SCRIPTGRAPH_ENABLED_WAIT_TIME_SECONDS)  # Client

    
    # Exit game mode
    helper.exit_game_mode(TestSuccessFailTuples.exit_game_mode)



if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Multiplayer_SimpleNetworkLevelEntity)
