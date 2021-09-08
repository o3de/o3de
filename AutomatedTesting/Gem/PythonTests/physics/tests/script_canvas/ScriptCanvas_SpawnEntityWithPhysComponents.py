"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# Test case ID : C6224408
# Test Case Title : Entity using PhysX nodes in Script Canvas can be spawned.


# fmt: off
class Tests:
    enter_game_mode      = ("Entered game mode",                "Failed to enter game mode")
    spawn_point_position = ("Spawn_Point is above the terrain", "Spawn_Point is below the terrain")
    Ball_0_found         = ("Ball_0 entity is valid",           "Ball_0 entity is not valid")
    Spawn_Point_found    = ("Spawn_Point entity is valid",      "Spawn_Point entity is not valid")
    Terrain_found        = ("Terrain entity is valid",          "Terrain entity is not valid")
    ball_spawned         = ("A ball has been spawned",          "A ball has not been spawned")
    exit_game_mode       = ("Exited game mode",                 "Couldn't exit game mode")

# fmt: on


def ScriptCanvas_SpawnEntityWithPhysComponents():
    """
    Summary:
    A spawner is set to spawn a sphere with downward velocity after a set amount of time. This action is controlled
        by a scriptcanvas.

    Level Description:
    Ball_0 - Existing only for user reference, a dynamic slice is taken from Ball_0 and used by the Spawn_point entity.
        Is set with velocity in the negative -z direction; has sphere shaped Collider, Rigid Body, Sphere shape, and
        Script Canvas (can be placed anywhere on the level). Velocity set to zero and script canvas added after dynamic
        slice taken.
    Spawn_Point - Activated by the associated Script canvas to spawn the associated dynamic slice; has spawner
    Terrain - Surface for spawned ball to bounce against; has terrain

    Script Canvas - The script canvas after a delay of 0.5 seconds tells the Spawn_Point entity to spawn.

    Expected Behavior:
    After a set amount of time dictated by the script canvas a ball will spawn and collide with the terrain component.

    Test Steps:
    1) Open Level
    2) Enter Game Mode
    3) Initialize and validate entities
    4) Set up handler for terrain collision
    5) Wait for terrain collision
    6) Log Results
    7) Exit Game Mode
    8) Close Editor

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
    import azlmbr.entity

    # Constants
    FLOAT_THRESHOLD = sys.float_info.epsilon
    TIMEOUT = 3.0

    # Global Variables
    id_list = []

    # Helper Functions
    class Collision:
        collision_on_terrain = False

    class Entity:
        def __init__(self, name, has_rigid_body):
            self.id = general.find_game_entity(name)
            self.name = name
            self.initial_position = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", self.id)
            id_list.append(self.id)

        class Tests:
            found = None

        def validate_entity(self):
            self.Tests.found = Tests.__dict__[self.name + "_found"]
            Report.critical_result(self.Tests.found, self.id.isValid())

    def spawn_point_position_check(spawn_point, terrain):
        position_valid = (
            spawn_point.initial_position.z > terrain.initial_position.z
        )
        Report.critical_result(Tests.spawn_point_position, position_valid)

    def on_terrain_collision(arg):
        if arg[0] not in id_list:  # ensures that the collision is with a new entity
            Collision.collision_on_terrain = True
            Report.info("Something new has collided with the Terrain")

    # Main Script
    helper.init_idle()
    # 1) Open Level
    helper.open_level("physics", "ScriptCanvas_SpawnEntityWithPhysComponents")

    # 2) Enter Game Mode
    helper.enter_game_mode(Tests.enter_game_mode)

    # 3) Initialize and validated entities
    ball_0 = Entity("Ball_0", True)
    spawn_point = Entity("Spawn_Point", False)
    terrain = Entity("Terrain", False)

    entity_list = [ball_0, spawn_point, terrain]
    for entity in entity_list:
        entity.validate_entity()

    spawn_point_position_check(spawn_point, terrain)

    # 4) Set up handler for terrain collision
    handler = azlmbr.physics.CollisionNotificationBusHandler()
    handler.connect(terrain.id)
    handler.add_callback("OnCollisionBegin", on_terrain_collision)

    # 5) Wait for terrain collision
    helper.wait_for_condition(lambda: Collision.collision_on_terrain, TIMEOUT)

    # 6) Log Results
    Report.result(Tests.ball_spawned, Collision.collision_on_terrain)

    # 7) Exit Game Mode
    helper.exit_game_mode(Tests.exit_game_mode)



if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(ScriptCanvas_SpawnEntityWithPhysComponents)
