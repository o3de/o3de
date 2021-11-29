"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# Test case ID : C5689518
# Test Case Title : PhysX entities collide with PhysX Terrain

# A ball is suspended slightly over PhysX Terrain to check that it collides when dropped


# fmt: off
class Tests():
    enter_game_mode      = ("Entered game mode",                "Failed to enter game mode")
    find_sphere          = ("Sphere entity found",              "Sphere entity not found")
    find_terrain         = ("Terrain found",                    "Terrain not found")
    sphere_above_terrain = ("Sphere position above ground",     "Sphere is not above the ground")
    time_out             = ("No time out occurred",             "A time out occurred, please validate level setup")
    touched_ground       = ("Touched ground before time out",   "Did not touch ground before time out")
    exit_game_mode       = ("Exited game mode",                 "Couldn't exit game mode")
# fmt: on


def Terrain_CollisionAgainstRigidBody():
    """
    Summary:
    Runs a test to make sure that a PhysX Rigid Body and Collider can successfully collide with a PhysX Terrain entity.

    Level Description:
    A sphere with rigid body and collider is positioned above a PhysX terrain. The terrain and sphere have the same
    collision group/layer assigned.

    Expected Outcome:
    Once game mode is entered, the sphere should fall toward and collide with the terrain.

    Steps:
    1) Open level and enter game mode
    2) Retrieve entities and positions
    3) Wait for sphere to collide with terrain OR time out
    4) Exit game mode
    5) Close the editor

    :return:
    """
    import os
    import sys
    import azlmbr.legacy.general as general
    import azlmbr.bus
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    # Global time out
    TIME_OUT = 1.0

    # 1) Open level / Enter game mode
    helper.init_idle()
    helper.open_level("Physics", "Terrain_CollisionAgainstRigidBody")
    helper.enter_game_mode(Tests.enter_game_mode)

    # 2) Retrieve entities and positions
    sphere_id = general.find_game_entity("PhysX_Sphere")
    Report.critical_result(Tests.find_sphere, sphere_id.IsValid())

    terrain_id = general.find_game_entity("PhysX_Terrain")
    Report.critical_result(Tests.find_terrain, terrain_id.IsValid())

    sphere_pos = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTM", sphere_id).GetPosition()
    terrain_pos = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTM", terrain_id).GetPosition()
    Report.info_vector3(sphere_pos, "Sphere:")
    Report.info_vector3(terrain_pos, "Terrain:")
    Report.critical_result(
        Tests.sphere_above_terrain,
        sphere_pos.z > terrain_pos.z,
        "Please make sure the sphere entity is set above the terrain",
    )

    # Enable gravity (just in case it is not enabled)
    if not azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "IsGravityEnabled", sphere_id):
        azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "SetGravityEnabled", sphere_id, True)

    class TouchGround:
        value = False

    # Collision event handler
    def on_collision_begin(args):
        other_id = args[0]
        if other_id.Equal(terrain_id):
            Report.info("Touched ground")
            TouchGround.value = True

    # Assign event handler to sphere
    handler = azlmbr.physics.CollisionNotificationBusHandler()
    handler.connect(sphere_id)
    handler.add_callback("OnCollisionBegin", on_collision_begin)

    # 3) Wait for the sphere to hit the ground OR time out
    test_completed = helper.wait_for_condition((lambda: TouchGround.value), TIME_OUT)

    Report.critical_result(Tests.time_out, test_completed)
    Report.result(Tests.touched_ground, TouchGround.value)

    # 4) Exit game mode
    helper.exit_game_mode(Tests.exit_game_mode)



if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Terrain_CollisionAgainstRigidBody)
