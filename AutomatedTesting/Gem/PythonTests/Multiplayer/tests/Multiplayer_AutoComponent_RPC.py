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
    found_lines =               ("Expected log lines were found",           "Expected log lines were not found")
    found_unexpected_lines =    ("Unexpected log lines were not found",     "Unexpected log lines were found")
# fmt: on


def Multiplayer_AutoComponent_RPC():
    r"""
    Summary:
    Runs a test to make sure that network input can be sent from the autonomous player, received by the authority, and processed

    Level Description:
    - Dynamic
        1. Although the level is empty, when the server and editor connect the server will spawn and replicate the player network prefab.
           a. The player network prefab has a NetworkTestPlayerComponent.AutoComponent and a script canvas attached which will listen for the CreateInput and ProcessInput events.  
               Print logs occur upon triggering the CreateInput and ProcessInput events along with their values; we are testing to make sure the expected events are values are recieved. 
    - Static
        1. This is an empty level. All the logic occurs on the Player.network.spawnable (see the above Dynamic description)
    
                    
    Expected Outcome:
    We should see editor logs stating that network input has been created and processed.
    However, if the script receives unexpected values for the Process event we will see print logs for bad data as well.
    
    :return:
    """
    import azlmbr.legacy.general as general
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import Tracer

    from editor_python_test_tools.utils import TestHelper as helper
    from ly_remote_console.remote_console_commands import RemoteConsole as RemoteConsole


    # looks for an expected line in a list of tracers lines
    # lines: the tracer list of lines to search. options are section_tracer.warnings, section_tracer.errors, section_tracer.asserts, section_tracer.prints
    # return: true if the line is found, otherwise false
    def find_expected_line(expected_line, lines):
        found_lines = [printInfo.message.strip() for printInfo in lines]
        return expected_line in found_lines

    def wait_for_critical_expected_line(expected_line, lines, time_out):
        helper.wait_for_condition(lambda : find_expected_line(expected_line, lines), time_out)
        Report.critical_result(("Found expected line: " + expected_line, "Failed to find expected line: " + expected_line), find_expected_line(expected_line, lines))

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
        wait_for_critical_expected_line('Script: AutoComponent_RPC: Sending client PlayerNumber 1', section_tracer.prints, EXPECTEDLINE_WAIT_TIME_SECONDS)
        wait_for_critical_expected_line("AutoComponent_RPC: I'm Player #1", section_tracer.prints, EXPECTEDLINE_WAIT_TIME_SECONDS)

    
    # Exit game mode
    helper.exit_game_mode(Tests.exit_game_mode)



if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Multiplayer_AutoComponent_RPC)
