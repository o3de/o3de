"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# Test case ID : C5689528
# Test Case Title : Create multiple entities each with one PhysX terrain component and verify that a warning
# is thrown to the user



# fmt: off
class Tests():
    enter_game_mode        = ("Entered game mode",               "Failed to enter game mode")
    warning_message_logged = ("The expected warning was logged", "The expected message was not logged")
    find_terrains          = ("All Terrains are found",          "All Terrains are not found")
    exit_game_mode         = ("Exited game mode",                "Couldn't exit game mode")
# fmt: on


def Terrain_MultipleTerrainComponentsWarning():

    """
    Summary:
    Create multiple entities each with one PhysX terrain component and verify that a warning is thrown to the user.

    Level Description:
    Terrain1 (entity) - Entity with PhysX Terrain component.
    Terrain2 (entity) - Entity with PhysX Terrain component.
    Terrain3 (entity) - Entity with PhysX Terrain component.

    Expected Behavior:
    We are verifying if the entities are valid.
    Additionally we are checking for the messages in log when 3 entities with terrain components are added.
    The log messages should be as follows:
    [Warning] (EditorTerrainComponent) - Multiple EditorTerrainComponents found in the editor scene on these entities:
    Terrain1
    Terrain2
    Terrain3

    Test Steps:
     1) Open level
     2) Enter game mode
     3) Look for warning
     4) Retrieve and validate entities
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
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper
    from editor_python_test_tools.utils import Tracer

    import azlmbr.legacy.general as general

    helper.init_idle()

    with Tracer() as warning_tracer:
        def has_terrain_warning():
            return warning_tracer.has_warnings and any(
                'EditorTerrainComponent' in warningInfo.window for warningInfo in warning_tracer.warnings)

        # 1) Open level
        helper.open_level("Physics", "Terrain_MultipleTerrainComponentsWarning")

        # 2) Enter game mode
        helper.enter_game_mode(Tests.enter_game_mode)

        # 3) Look for warning
        helper.wait_for_condition(has_terrain_warning, 1.0)
        Report.result(Tests.warning_message_logged, has_terrain_warning())

        # 4) Retrieve and validate entities
        all_terrains_found = True
        for index in range(1, 4):
            entity_name = "Terrain{}".format(index)
            terrain_id = general.find_game_entity(entity_name)
            if not terrain_id.IsValid():
                all_terrains_found = False
        Report.critical_result(Tests.find_terrains, all_terrains_found)

        # 5) Exit game mode
        helper.exit_game_mode(Tests.exit_game_mode)



if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Terrain_MultipleTerrainComponentsWarning)
