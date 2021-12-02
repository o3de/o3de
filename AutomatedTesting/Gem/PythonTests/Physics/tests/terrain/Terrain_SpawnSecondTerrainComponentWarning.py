"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# Test case ID : C5689531
# Test Case Title : Check that when you add a spawner component to a level to spawn a
# terrain and also add a terrain component explicitly, no crash happens



# fmt: off
class Tests:
    enter_game_mode        = ("Entered game mode",               "Failed to enter game mode")
    find_terrain           = ("Terrain found",                   "Terrain not found")
    find_spawner           = ("Spawner found",                   "Spawner not found")
    find_terrain_slice     = ("Terrain slice found",             "Terrain slice not found")
    warning_message_logged = ("The expected warning was logged", "The expected message was not logged")
    exit_game_mode         = ("Exited game mode",                "Couldn't exit game mode")
# fmt: on


def Terrain_SpawnSecondTerrainComponentWarning():

    """
    Summary:
    Check that when you add a spawner component to a level to spawn a terrain and also
    add a terrain component explicitly, no crash happens

    Level Description:
    SpawnerEntity (entity) - Entity with Spawner component. Attached to it is a dynamic slice file which is used
                             to create an entity "TerrainSlice" with a PhysX Terrain component.
                             Spawn On Activate option is checked.
    TerrainEntity (entity) - Entity with PhysX Terrain component.

    Expected Behavior:
    We are verifying if the entities are valid.
    We are checking for the messages in log as multiple terrains are added to the level.
    The log messages should be as follows:
    [Warning] (TerrainComponent) - Multiple TerrainComponents found in the scene on these entities:
    TerrainEntity
    TerrainSlice

    Test Steps:
     1) Open level
     2) Enter game mode
     3) Retrieve and validate entities
     4) Wait for WAIT_TIME to check if any crash happened
     5) Check if the TerrainSlice spawner has spawned
     6) Look for warning
     7) Exit game mode
     8) Close the editor


    Note:
    - This test file must be called from the Open 3D Engine Editor command terminal
    - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    import os
    import sys
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper
    from editor_python_test_tools.utils import Tracer

    import azlmbr.legacy.general as general

    # Constants
    WAIT_TIME = 1.0

    helper.init_idle()

    with Tracer() as warning_tracer:
        def has_terrain_warning():
            return warning_tracer.has_warnings and any(
                'TerrainComponent' in warningInfo.window for warningInfo in warning_tracer.warnings)

        # 1) Open level
        helper.open_level("Physics", "Terrain_SpawnSecondTerrainComponentWarning")

        # 2) Enter game mode
        helper.enter_game_mode(Tests.enter_game_mode)

        # 3) Retrieve and validate entities
        terrain_id = general.find_game_entity("TerrainEntity")
        Report.critical_result(Tests.find_terrain, terrain_id.IsValid())
        spawner_id = general.find_game_entity("SpawnerEntity")
        Report.critical_result(Tests.find_spawner, spawner_id.IsValid())

        # 4) Wait for WAIT_TIME to check if any crash happened
        general.idle_wait(WAIT_TIME)

        # 5) Check if the TerrainSlice spawner has spawned
        terrain_slice_id = general.find_game_entity("TerrainSlice")
        Report.critical_result(Tests.find_terrain_slice, terrain_slice_id.IsValid())

        # 6) Look for warning
        helper.wait_for_condition(has_terrain_warning, WAIT_TIME)
        Report.result(Tests.warning_message_logged, has_terrain_warning())

        # 7) Exit game mode
        helper.exit_game_mode(Tests.exit_game_mode)



if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Terrain_SpawnSecondTerrainComponentWarning)
