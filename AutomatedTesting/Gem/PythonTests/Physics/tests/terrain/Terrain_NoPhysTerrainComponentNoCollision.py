"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# Test case ID : C3510642
# Test Case Title : Check that when no physX terrain component is added, collision of a PhysX object
# with terrain does not work. Consequently, PhysX material assignment to terrain cannot be tested.



# fmt: off
class Tests():
    enter_game_mode             = ("Entered game mode",               "Failed to enter game mode")
    find_box                    = ("Box entity found",                "Box entity not found")
    find_bumper                 = ("Bumper box found",                "Bumper box not found")
    box_above_terrain           = ("The tester box is above terrain", "The test box is not higher than the terrain")
    bumper_below_terrain        = ("The bumper is below terrain",     "The bumper is not lower than the terrain")
    gravity_works               = ("Box fell",                        "Box did not fall")
    falls_below_terrain_height  = ("Box is below terrain",            "Box did not fall below terrain before timeout")
    collision_underground       = ("Box collided underground",        "Box did not collide underground before timeout")
    exit_game_mode              = ("Exited game mode",                "Couldn't exit game mode")
# fmt: on


def Terrain_NoPhysTerrainComponentNoCollision():
    """
    Summary:
    Runs an automated test to ensure that when no PhysX Terrain component is added,
    PhysX objects will not collide with terrain.

    Level Description:
    Box (entity) - suspended over the terrain with gravity enabled;  contains a box mesh,
        PhysX Collider (Box shape), and PhysX RigidBody
    Bumper (entity) - suspended under the terrain with gravity disabled; contains box mesh,
        PhysX Collider (Box shape), and PhysX RigidBody

    Expected Behavior:
    When game mode is entered, the Box entity will experience gravity and fall toward the terrain.
    Since there is no PhysX Terrain component in the level, it should fall through the terrain.
    Once it passes through the terrain, it will collide with the Bumper entity in order to prove that
    it has passed the terrain.

    Test Steps:
     1) Open level
     2) Enter game mode
     3) Find the entities
     4) Get the starting z position of the boxes
     5) Check and report that the entities are at the correct heights
     6) Check that the gravity works and the box falls
     7) Check that the box hits the trigger and is below the terrain
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
    import azlmbr.legacy.general as general
    import azlmbr.bus

    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    # Constants
    TIMEOUT = 2.0
    TERRAIN_HEIGHT = 32.0  # Default height of the terrain
    MIN_BELOW_TERRAIN = 0.5  # Minimum height below terrain the box must be in order to be 'under' it

    helper.init_idle()
    # 1) Open level
    helper.open_level("Physics", "Terrain_NoPhysTerrainComponentNoCollision")

    # 2) Enter game mode
    helper.enter_game_mode(Tests.enter_game_mode)

    # 3) Find the entities
    box_id = general.find_game_entity("Box")
    Report.critical_result(Tests.find_box, box_id.IsValid())

    bumper_id = general.find_game_entity("Bumper")
    Report.critical_result(Tests.find_bumper, bumper_id.IsValid())

    # 4) Get the starting z position of the boxes
    class Box:
        """
        Class to hold boolean values for test checks.
        Attributes:
            start_position_z: The initial z position of the box
            position_z      : The z position of the box
            fell            : When the box falls any distance below its original position, the value should be set True
            below_terrain   : When the box falls below the specified terrain height, the value should be set True
        """

        start_position_z = None
        position_z = None
        fell = False
        below_terrain = False

    Box.start_position_z = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldZ", box_id)
    bumper_start_z = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldZ", bumper_id)

    # 5) Check that the test box is above the terrain and the bumper box is below terrain
    Report.info("Terrain Height: {}".format(TERRAIN_HEIGHT))
    Report.info("Box start height: {}".format(Box.start_position_z))
    Report.result(Tests.box_above_terrain, Box.start_position_z > TERRAIN_HEIGHT)
    Report.info("Bumper start height: {}".format(bumper_start_z))
    Report.result(Tests.bumper_below_terrain, bumper_start_z < TERRAIN_HEIGHT)

    # 6) Check that the gravity works and the box falls
    def box_fell():
        if not Box.fell:
            Box.position_z = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldZ", box_id)
            if Box.position_z < Box.start_position_z:
                Report.info("Box position is now lower than the starting position")
                Box.fell = True
        return Box.fell

    helper.wait_for_condition(box_fell, TIMEOUT)
    Report.result(Tests.gravity_works, Box.fell)

    # 7) Check that the box hits the trigger and is below the terrain
    # Setup for collision check
    class BumperTriggerEntered:
        value = False

    def on_collision_begin(args):
        other_id = args[0]
        if other_id.Equal(bumper_id):
            BumperTriggerEntered.value = True

    handler = azlmbr.physics.CollisionNotificationBusHandler()
    handler.connect(box_id)
    handler.add_callback("OnCollisionBegin", on_collision_begin)

    def box_below_terrain():
        if not Box.below_terrain:
            Box.position_z = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldZ", box_id)
            if Box.position_z < (TERRAIN_HEIGHT - MIN_BELOW_TERRAIN):
                Report.info("Box position is now below the terrain")
                Box.below_terrain = True
        return Box.below_terrain

    def box_below_and_trigger_entered():
        return box_below_terrain() and BumperTriggerEntered.value

    helper.wait_for_condition(box_below_and_trigger_entered, TIMEOUT)
    Report.result(Tests.collision_underground, BumperTriggerEntered.value)
    Report.result(Tests.falls_below_terrain_height, Box.below_terrain)

    # 8) Exit game mode
    helper.exit_game_mode(Tests.exit_game_mode)



if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Terrain_NoPhysTerrainComponentNoCollision)
