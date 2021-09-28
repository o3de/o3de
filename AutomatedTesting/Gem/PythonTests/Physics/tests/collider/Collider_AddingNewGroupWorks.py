"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# Test case ID : C4976227
# Test Case Title : Validate that a Collision Group can be added

# Level has entity with custom collision group added.
# If level enters game mode, collision group addition is validated.

# fmt: off
class Tests:
    enter_game_mode = ("Entered game mode",                  "Failed to enter game mode")
    find_sphere     = ("Sphere entity found",                "Sphere entity not found")
    collision_group = ("Collision group addition validated", "Collision group addition not valid")
    exit_game_mode  = ("Exited game mode",                   "Couldn't exit game mode")
# fmt: on


def Collider_AddingNewGroupWorks():
    """
    Summary:
    Runs an automated test to ensure that a collision group can be added.

    Level Description:
    Sphere (Entity)  - PhysX Collider(shape:sphere): Collision Layer (Default), Collides With (Test_Group)

    Test_Group (Collision Group) - Collision Group that is custom made for this test. 
        Requires a custom ".physxconfiguration" file in addition to the level file

    Expected Behavior:
    When game mode is entered the entity id should be valid and position should be found

    Test Steps:
    1) Open Level
    2) Enter game mode
    3) Validate entities
    4) Exit game mode
    5) Close Editor

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

    import azlmbr.legacy.general as general
    import azlmbr.bus

    helper.init_idle()
    # 1) Open Level
    helper.open_level("Physics", "Collider_AddingNewGroupWorks")

    # 2) Enter game mode
    helper.enter_game_mode(Tests.enter_game_mode)

    # 3) Validate entities
    sphere_id = general.find_game_entity("Sphere")

    Report.result(Tests.find_sphere, sphere_id.IsValid())

    sphere_position = None
    sphere_position = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", sphere_id)

    Report.result(Tests.collision_group, sphere_id.isValid() and sphere_position != None)

    # 4) Exit game mode
    helper.exit_game_mode(Tests.exit_game_mode)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Collider_AddingNewGroupWorks)
