"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# Test Case Title : Check that the four network RPCs can be sent and received


# fmt: off
class Tests():
    enter_game_mode =           ("Entered game mode",                       "Failed to enter game mode")
    exit_game_mode =            ("Exited game mode",                        "Couldn't exit game mode")
    find_network_player =       ("Found network player",                    "Couldn't find network player")
# fmt: on


def Multiplayer_AutoComponent_RPC():
    r"""
    Summary:
    Runs a test to make sure that RPCs can be sent and received via script canvas

    Level Description:
    - Dynamic
        1. Although the level is nearly empty, when the server and editor connect the server will spawn and replicate the player network prefab.
           a. The player network prefab has a NetworkTestPlayerComponent.AutoComponent and a script canvas attached which sends and receives various RPCs.  
               Print logs occur upon sending and receiving the RPCs; we are testing to make sure the expected events and values are received. 
    - Static
        1. NetLevelEntity. This is a networked entity which has a script attached. Used for cross-entity communication. The net-player prefab will send this level entity Server->Authority RPCs
    
                    
    Expected Outcome:
    We should see editor logs stating that RPCs have been sent and received.
    However, if the script receives unexpected values for the Process event we will see print logs for bad data as well.
    
    :return:
    """
    import azlmbr.legacy.general as general
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import Tracer

    from editor_python_test_tools.utils import TestHelper as helper
    from ly_remote_console.remote_console_commands import RemoteConsole as RemoteConsole

    level_name = "AutoComponent_RPC"
    player_prefab_name = "Player"
    player_prefab_path = f"levels/multiplayer/{level_name}/{player_prefab_name}.network.spawnable"

    helper.init_idle()

    # 1) Open Level
    helper.open_level("Multiplayer", level_name)

    with Tracer() as section_tracer:
        # 2) Enter game mode
        helper.multiplayer_enter_game_mode(Tests.enter_game_mode, player_prefab_path.lower())

        # 3) Make sure the network player was spawned
        player_id = general.find_game_entity(player_prefab_name)
        Report.critical_result(Tests.find_network_player, player_id.IsValid())

        # 4) Check the editor logs for expected and unexpected log output
        EXPECTEDLINE_WAIT_TIME_SECONDS = 1.0
        helper.wait_for_critical_expected_line('EditorServer', 'Script: AutoComponent_RPC: Sending client PlayerNumber 1', section_tracer.prints, EXPECTEDLINE_WAIT_TIME_SECONDS)
        helper.wait_for_critical_expected_line('Script', "AutoComponent_RPC: I'm Player #1", section_tracer.prints, EXPECTEDLINE_WAIT_TIME_SECONDS)

    
    # Exit game mode
    helper.exit_game_mode(Tests.exit_game_mode)



if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Multiplayer_AutoComponent_RPC)
