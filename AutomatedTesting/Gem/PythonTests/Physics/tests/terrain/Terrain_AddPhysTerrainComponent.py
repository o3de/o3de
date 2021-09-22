"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# Test case ID : C5689522
# Test Case Title : Create an entity with PhysX terrain. Add another PhysX terrain and verify that
# you are able to add it without any crash or error.



# fmt: off
class Tests():
    enter_game_mode        = ("Entered game mode",               "Failed to enter game mode")
    find_terrain_1         = ("Terrain 1 found",                 "Terrain 1 not found")
    find_terrain_2         = ("Terrain 2 found",                 "Terrain 2 not found")
    warning_message_logged = ("The expected warning was logged", "The expected message was not logged")
    exit_game_mode         = ("Exited game mode",                "Couldn't exit game mode")
# fmt: on


def Terrain_AddPhysTerrainComponent():
    """
    Summary:
    Runs an automated test by creating an entity with physx terrain and add another physx terrain and verify that
    you are able to add it without any crash or error.

    Level Description:
    Terrain1 (entity) - contains Physx Terrain, status is inactive in Editor
    Terrain2 (entity) - contains Physx Terrain, status is inactive in Editor

    Expected Behavior:
    When game mode is entered, check the console to find this warning :
    [Warning] (EditorTerrainComponent) - Multiple EditorTerrainComponents found in the editor scene on these entities:
    Terrain1
    Terrain2
    The warning will list down all the entities on which the terrain component is currently present.

    Test Steps:
     1) Open level
     2) Enter game mode
     3) Validate entities
     4) Activate entities
     5) Look for warning
     6) Exit game mode
     7) Close the editor

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
        helper.open_level("Physics", "Terrain_AddPhysTerrainComponent")

        # 2) Enter game mode
        helper.enter_game_mode(Tests.enter_game_mode)

        # 3) Validate entities
        terrain_id1 = general.find_game_entity("Terrain1")
        terrain_id2 = general.find_game_entity("Terrain2")

        Report.result(Tests.find_terrain_1, terrain_id1.IsValid())
        Report.result(Tests.find_terrain_2, terrain_id2.IsValid())

        # 4) Activate entities
        azlmbr.entity.GameEntityContextRequestBus(azlmbr.bus.Broadcast, "ActivateGameEntity", terrain_id1)
        azlmbr.entity.GameEntityContextRequestBus(azlmbr.bus.Broadcast, "ActivateGameEntity", terrain_id2)

        # 5) Look for warning
        helper.wait_for_condition(has_terrain_warning, 1.0)
        Report.result(Tests.warning_message_logged, has_terrain_warning())

        # 6) Exit game mode
        helper.exit_game_mode(Tests.exit_game_mode)



if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Terrain_AddPhysTerrainComponent)
