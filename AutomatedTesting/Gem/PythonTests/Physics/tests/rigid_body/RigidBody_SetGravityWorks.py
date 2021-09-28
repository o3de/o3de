"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# Test case ID : C14976307
# Test Case Title : Check that Set Gravity Enabled works on an entity with gravity that starts as disabled



# fmt: off
class Tests():
    enter_game_mode            = ("Entered game mode",              "Failed to enter game mode")
    find_entities              = ("Entities are found",             "Entities are not found")
    gravity_initially_disabled = ("Gravity was initially disabled", "Gravity was initially enabled")
    gravity_enabled            = ("Enabled gravity successfully",   "Failed to enable gravity")
    collision_occured          = ("Sphere collided with terrain",   "Sphere did not collide with terrain")
    exit_game_mode             = ("Exited game mode",               "Couldn't exit game mode")
# fmt: on


def RigidBody_SetGravityWorks():

    """
    Summary:
    Check that Set Gravity Enabled works on an entity with gravity that starts as disabled

    Level Description:
    Terrain (entity) - Terrain entity is created in the level
    Sphere (entity) - Entity with PhysX rigid body, mesh and collider with gravity disabled placed above
    the terrain

    Expected Behavior:
    After 5 seconds, when SetGravity is called, the entity falls to the ground
    We are checking if entities are valid and enabling the gravity after 5 seconds in game mode to check if ball
    falls on the terrain.

    Test Steps:
     1) Open level
     2) Enter game mode
     3) Retrieve and validate entities
     4) Gravity check for entity
     5) Enabling gravity after 5 seconds
     6) Adding collision handlers for terrain
     7) Checking if the object collides with terrain
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
    TIME_OUT = 3.0
    WAIT_TIME = 5.0

    helper.init_idle()

    # 1) Open level
    helper.open_level("Physics", "RigidBody_SetGravityWorks")

    # 2) Enter game mode
    helper.enter_game_mode(Tests.enter_game_mode)

    # 3) Retrieve and validate entities
    terrain_id = general.find_game_entity("Terrain")
    sphere_id = general.find_game_entity("Sphere")
    Report.critical_result(Tests.find_entities, terrain_id.IsValid() and sphere_id.IsValid())

    sphere_gravity_enabled = False
    class Sphere:
        sphere_collision_occured = False

    # 4) Gravity check for entities
    sphere_gravity_enabled = azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "IsGravityEnabled", sphere_id)
    Report.result(Tests.gravity_initially_disabled, not sphere_gravity_enabled)

    # 5) Adding collision handlers for terrain
    def on_collision_begin(args):
        other_id = args[0]
        if other_id.Equal(sphere_id):
            Report.info("Sphere collided with the terrain")
            Sphere.sphere_collision_occured = True

    handler = azlmbr.physics.CollisionNotificationBusHandler()
    handler.connect(terrain_id)
    handler.add_callback("OnCollisionBegin", on_collision_begin)

    # 6) Enabling gravity after 5 seconds
    general.idle_wait(WAIT_TIME)
    azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "SetGravityEnabled", sphere_id, True)
    sphere_gravity_enabled = azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "IsGravityEnabled", sphere_id)
    Report.result(Tests.gravity_enabled, sphere_gravity_enabled)

    # 7) Checking if the object collides with terrain
    helper.wait_for_condition(lambda: Sphere.sphere_collision_occured, TIME_OUT)
    Report.result(Tests.collision_occured, Sphere.sphere_collision_occured)

    # 8) Exit game mode
    helper.exit_game_mode(Tests.exit_game_mode)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(RigidBody_SetGravityWorks)
