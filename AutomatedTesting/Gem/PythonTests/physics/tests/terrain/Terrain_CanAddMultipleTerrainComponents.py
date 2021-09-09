"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# Test case ID : C5689524
# Test Case Title : Create multiple entities each with one or more terrain components and verify that you are
# able to successfully add the PhysX Terrain components to them.



# fmt: off
class Tests():
    enter_game_mode        = ("Entered game mode",               "Failed to enter game mode")
    all_terrains_found     = ("Entities found",                  "Entities not found")
    warning_message_logged = ("The expected warning was logged", "The expected message was not logged")
    exit_game_mode         = ("Exited game mode",                "Couldn't exit game mode")
# fmt: on


def Terrain_CanAddMultipleTerrainComponents():
    """
    Summary:
    Runs an automated test by creating multiple entities each with one or more terrain components and verify that you are
    able to successfully add the PhysX Terrain components to them.

    Level Description:
    Terrain1 (entity) - contains Physx Terrain, Status in inactive in Editor
    Terrain2 (entity) - contains Physx Terrain, Status in inactive in Editor
    Terrain3 (entity) - contains Physx Terrain, Status in inactive in Editor
    Terrain4 (entity) - contains Physx Terrain, Status in inactive in Editor

    Expected Behavior:
    When game mode is entered, check the console to find this warning :
    [Warning] (EditorTerrainComponent) - Multiple EditorTerrainComponents found in the editor scene on these entities:
    Terrain1
    Terrain2
    Terrain3
    Terrain4
    The warning will list down all the entities on which the terrain component is currently present.

    Test Steps:
     1) Open level
     2) Enter game mode
     3) Activate and Validate entities
     4) Look for warning
     5) Exit game mode
     6) Close the editor

    Note:
    - This test file must be called from the Open 3D Engine Editor command terminal
    - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    import os
    import sys
    import azlmbr.legacy.general as general

    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper
    from editor_python_test_tools.utils import Tracer

    import azlmbr.bus

    helper.init_idle()

    with Tracer() as warning_tracer:
        def has_terrain_warning():
            return warning_tracer.has_warnings and any(
                'EditorTerrainComponent' in warningInfo.window for warningInfo in warning_tracer.warnings)

        # 1) Open level
        helper.open_level("Physics", "Terrain_CanAddMultipleTerrainComponents")

        # 2) Enter game mode
        helper.enter_game_mode(Tests.enter_game_mode)

        # 3) Activate and Validate entities
        terrains = ("Terrain1", "Terrain2", "Terrain3", "Terrain4")
        all_terrains_found = True
        for terrain in terrains:
            terrain_id = general.find_game_entity(terrain)
            azlmbr.entity.GameEntityContextRequestBus(azlmbr.bus.Broadcast, "ActivateGameEntity", terrain_id)
            if not terrain_id.IsValid():
                all_terrains_found = False
        Report.critical_result(Tests.all_terrains_found, all_terrains_found)

        # 4) Look for warning
        helper.wait_for_condition(has_terrain_warning, 1.0)
        Report.result(Tests.warning_message_logged, has_terrain_warning())

        # 5) Exit game mode
        helper.exit_game_mode(Tests.exit_game_mode)



if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Terrain_CanAddMultipleTerrainComponents)
