"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# Test case ID : C4976242
# Test Case Title : Assign same collision layer and same collision group to two entities and
# verify that they collide or not


# fmt: off
class Tests():
    enter_game_mode            = ("Entered game mode",                      "Failed to enter game mode")
    find_moving                = ("Moving entity found",                    "Moving entity not found")
    find_stationary            = ("Stationary entity found",                "Stationary entity not found")
    find_terrain               = ("Terrain entity found",                   "Terrain entity not found")
    stationary_above_terrain   = ("Stationary is above terrain",            "Stationary is not above terrain")
    moving_above_stationary    = ("Moving is above stationary",             "Moving is not above stationary")
    gravity_works              = ("Moving Sphere fell down",                "Moving Sphere did not fall")
    collisions                 = ("Collision occurred in between entities", "Collision did not occur between entities")
    falls_below_terrain_height = ("Moving is below terrain",                "Moving did not fall below terrain before timeout")
    exit_game_mode             = ("Exited game mode",                       "Couldn't exit game mode")
# fmt: on


def Collider_SameCollisionGroupSameLayerCollide():

    """
    Summary:
    Open a Project that already has two entities with same collision layer and same collision group and verify collision

    Level Description:
    Moving and Stationary entities are created in level with same collision layer and same collision group.
    Moving entity is placed above the Stationary entity.Terrain is placed below the Stationary entity.
    So Moving and Stationary entities collide with each other and they go through terrain after collision.

    Expected Behavior:
    The Moving and Stationary entities should collide with each other.After Collision,they go through terrain.

    Test Steps:
     1) Open level and Enter game mode
     2) Retrieve and validate Entities
     3) Get the starting z position of the Moving entity,Stationary entity and Terrain
     4) Check and report that the entities are at the correct heights before collision
     5) Check that the gravity works and the Moving entity falls down
     6) Check Spheres collide only with each other, but not with terrain
     7) Check Moving Entity should be below terrain after collision
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
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    import azlmbr.legacy.general as general
    import azlmbr.bus

    # Constants
    TIMEOUT = 2.0
    TERRAIN_HEIGHT = 32.0  # Default height of the terrain
    MIN_BELOW_TERRAIN = 0.5  # Minimum height below terrain the sphere must be in order to be 'under' it
    CLOSE_ENOUGH_THRESHOLD = 0.0001

    helper.init_idle()

    # 1) Open level and Enter game mode
    helper.open_level("Physics", "Collider_SameCollisionGroupSameLayerCollide")
    helper.enter_game_mode(Tests.enter_game_mode)

    # 2) Retrieve and validate Entities
    moving_id = general.find_game_entity("Sphere_Moving")
    Report.critical_result(Tests.find_moving, moving_id.IsValid())

    stationary_id = general.find_game_entity("Sphere_Stationary")
    Report.critical_result(Tests.find_stationary, stationary_id.IsValid())

    terrain_id = general.find_game_entity("Terrain")
    Report.critical_result(Tests.find_terrain, terrain_id.IsValid())

    # 3) Get the starting z position of the Moving entity,Stationary entity and Terrain
    class Sphere:
        """
        Class to hold values for test checks.
        Attributes:
            start_position_z: The initial z position of the sphere
            position_z      : The z position of the sphere
            fell            : When the sphere falls any distance below its original position, the value should be set True
            below_terrain   : When the box falls below the specified terrain height, the value should be set True
        """

        start_position_z = None
        position_z = None
        fell = False
        below_terrain = False

    Sphere.start_position_z = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldZ", moving_id)
    stationary_start_z = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldZ", stationary_id)

    # 4)Check and report that the entities are at the correct heights before collision
    Report.info(
        "Terrain Height: {} \n Stationary Sphere height: {} \n Moving Sphere height: {}".format(
            TERRAIN_HEIGHT, stationary_start_z, Sphere.start_position_z
        )
    )
    Report.result(Tests.stationary_above_terrain, TERRAIN_HEIGHT < (stationary_start_z - CLOSE_ENOUGH_THRESHOLD))
    Report.result(
        Tests.moving_above_stationary, stationary_start_z < (Sphere.start_position_z - CLOSE_ENOUGH_THRESHOLD)
    )

    # 5)Check that the gravity works and the Moving entity falls down
    def sphere_fell():
        if not Sphere.fell:
            Sphere.position_z = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldZ", moving_id)
            if Sphere.position_z < (Sphere.start_position_z - CLOSE_ENOUGH_THRESHOLD):
                Report.info("Sphere position is now lower than the starting position")
                Sphere.fell = True
        return Sphere.fell

    helper.wait_for_condition(sphere_fell, TIMEOUT)
    Report.result(Tests.gravity_works, Sphere.fell)

    # 6) Check Spheres collide only with each other, but not with terrain
    class Collision:
        entity_collision = False
        terrain_collision = False

    class CollisionHandler:
        def __init__(self, id, func):
            self.id = id
            self.func = func
            self.create_collision_handler()

        def on_collision_begin(self, args):
            self.func(args[0])

        def create_collision_handler(self):
            self.handler = azlmbr.physics.CollisionNotificationBusHandler()
            self.handler.connect(self.id)
            self.handler.add_callback("OnCollisionBegin", self.on_collision_begin)

    def on_collision_terrain(other_id):
        Collision.terrain_collision = True
        Report.info("Collision occured in between Moving or Stationary entity with Terrain")

    def on_moving_entity_collision(other_id):
        if other_id.Equal(stationary_id):
            Collision.entity_collision = True

    # collision handler for entities
    CollisionHandler(terrain_id, on_collision_terrain)
    CollisionHandler(moving_id, on_moving_entity_collision)
    # wait till timeout to check for any collisions happening in the level
    helper.wait_for_condition(lambda: Collision.entity_collision, TIMEOUT)
    Report.result(Tests.collisions, Collision.entity_collision and not Collision.terrain_collision)

    # 7)Check Moving Entity should be below terrain after collision
    def sphere_below_terrain():
        if not Sphere.below_terrain:
            Sphere.position_z = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldZ", moving_id)
            if Sphere.position_z < (TERRAIN_HEIGHT - MIN_BELOW_TERRAIN):
                Sphere.below_terrain = True
        return Sphere.below_terrain

    sphere_under_terrain = helper.wait_for_condition(sphere_below_terrain, TIMEOUT)
    Report.result(Tests.falls_below_terrain_height, sphere_under_terrain)

    # 8) Exit game mode
    helper.exit_game_mode(Tests.exit_game_mode)



if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Collider_SameCollisionGroupSameLayerCollide)
