"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# Test case ID : C4976243
# Test Case Title : Assign different collision layers and same collision group
# (such that this group has both these collision layers enabled) to two entities and verify that they collide



# fmt: off
class Tests():
    enter_game_mode     = ("Entered game mode",            "Failed to enter game mode")
    find_terrain        = ("Terrain found",                "Terrain not found")
    find_entity1        = ("Entity1 found",                "Entity1 not found")
    find_entity2        = ("Entity2 found",                "Entity2 not found")
    gravity_enabled     = ("Gravity is enabled",           "Gravity is disabled")
    gravity_disabled    = ("Gravity is disabled",          "Gravity is enabled")
    collision_occurance = ("Entity1 and Entity2 collided", "Entity1 and Entity2 did not collide")
    exit_game_mode      = ("Exited game mode",             "Couldn't exit game mode")
# fmt: on


def Collider_SameCollisionGroupDiffLayersCollide():

    """
    Summary:
    Assign different collision layers and same collision group (such that this group has both these
    collision layers enabled) to two entities and verify that they collide

    Level Description:
    Entity1 (entity) - Entity with components PhysX Rigid Body, PhysX Collider, Terrain and Rendering Mesh
                       "Collision Layer" as "A" and "Collides with" as "B" with gravity enabled.
                       Entity1 is placed exactly above Terrain and Entity2 along z axis
    Entity2 (entity) - Entity with components PhysX Rigid Body, PhysX Collider, Terrain and Rendering Mesh
                       "Collision Layer" as "Default" and "Collides with" as "B" with gravity disabled
                       Entity2 is placed exactly in between the Terrain and Entity1 along z axis
    Terrain (entity) - Entity with Terrain component.
                       "Collision Layer" as "Default" and "Collides with" as "All"

    Expected Behavior:
    Created entities should collide with each other.
    We are checking created entities collided (Entity1 and Entity2)

    Test Steps:
     1) Open level
     2) Enter game mode
     3) Retrieve and validate entities
     4) Check if gravity is enabled for entity1 and disabled for entity2
     5) Create collision event handlers
     6) Check for collisions between entities
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

    import azlmbr.legacy.general as general
    import azlmbr.bus

    # Constants
    TIMEOUT = 2  # waits for 2 secs to verify if the collision occured

    helper.init_idle()

    # 1) Open level
    helper.open_level("Physics", "Collider_SameCollisionGroupDiffLayersCollide")

    # 2) Enter game mode
    helper.enter_game_mode(Tests.enter_game_mode)

    # 3) Retrieve and validate entities
    terrain_id = general.find_game_entity("Terrain")
    Report.info(dir(terrain_id))
    entity1_id = general.find_game_entity("Entity1")
    entity2_id = general.find_game_entity("Entity2")
    Report.critical_result(Tests.find_terrain, terrain_id.IsValid())
    Report.critical_result(Tests.find_entity1, entity1_id.IsValid())
    Report.critical_result(Tests.find_entity2, entity2_id.IsValid())

    # 4) Check if gravity is enabled for entity1 and disabled for entity2
    is_gravity_enabled = azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "IsGravityEnabled", entity1_id)
    Report.info("Gravity check for entity1")
    Report.result(Tests.gravity_enabled, is_gravity_enabled)
    is_gravity_enabled = azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "IsGravityEnabled", entity2_id)
    Report.info("Gravity check for entity2")
    Report.result(Tests.gravity_disabled, not is_gravity_enabled)

    class Collision:
        entity_collision = False

    # 5) Create collision event handler
    def on_collision_begin(args):
        if args[0].Equal(entity1_id):
            Report.info("Collision occurred")
            Collision.entity_collision = True

    handler = azlmbr.physics.CollisionNotificationBusHandler()
    handler.connect(entity2_id)
    handler.add_callback("OnCollisionBegin", on_collision_begin)

    # 6) Check for collisions between entities
    helper.wait_for_condition(lambda: Collision.entity_collision, TIMEOUT)
    Report.critical_result(Tests.collision_occurance, Collision.entity_collision)

    # 7) Exit game mode
    helper.exit_game_mode(Tests.exit_game_mode)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Collider_SameCollisionGroupDiffLayersCollide)
