"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# Test case ID : C4976204
# Test Case Title : Verify that when Start Asleep is checked, the object in air does not fall down due to
# gravity or does not start moving with initial linear velocity assigned to it when switched to game mode



# fmt: off
class Tests():
    enter_game_mode       = ("Entered game mode",              "Failed to enter game mode")
    find_terrain          = ("Terrain found",                  "Terrain not found")
    find_sphere           = ("Sphere found",                   "Sphere not found")
    gravity_enabled       = ("Gravity is enabled",             "Gravity is disabled")
    start_asleep_enabled  = ("Start asleep is enabled",        "Start asleep is disabled")
    sphere_position_fixed = ("Sphere position did not change", "Sphere position changed")
    exit_game_mode        = ("Exited game mode",               "Couldn't exit game mode")
# fmt: on


def RigidBody_StartAsleepWorks():

    """
    Summary:
    Verify that when Start Asleep is checked, the object in air does not fall down due to gravity or
    does not start moving with initial linear velocity assigned to it when switched to game mode

    Level Description:
    Terrain (entity) - Entity with PhysX Terrain component
    Sphere (entity) - Entity with components PhysX Rigid Body, PhysX Collider and Rendering Mesh (sphere shape)
                      initial velocity in x direction - 5 m/s
                      gravity enabled, Start asleep enabled.

    Expected Behavior:
    Nothing happens. The sphere is in its position in the air.
    We are checking if the sphere is in the same position as it was earlier.

    Test Steps:
     1) Open level
     2) Enter game mode
     3) Retrieve and validate entities
     4) Get the initial x,y,z positions of the sphere
     5) Check gravity, start asleep values for the entity
     6) Wait until timeout to check if sphere position changed
     7) Check the final x, y, z positions of the sphere
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
    TIME_OUT = 3.0  # waits for 3 seconds to verify if the sphere moved

    helper.init_idle()

    # 1) Open level
    helper.open_level("Physics", "RigidBody_StartAsleepWorks")

    # 2) Enter game mode
    helper.enter_game_mode(Tests.enter_game_mode)

    # 3) Retrieve and validate entities
    terrain_id = general.find_game_entity("Terrain")
    sphere_id = general.find_game_entity("Sphere")
    Report.critical_result(Tests.find_terrain, terrain_id.IsValid())
    Report.critical_result(Tests.find_sphere, sphere_id.IsValid())

    # 4) Get the initial x,y,z positions of the sphere
    init_sphere_pos = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", sphere_id)

    # 5) Check gravity, start asleep values for the entity
    is_gravity_enabled = azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "IsGravityEnabled", sphere_id)
    Report.critical_result(Tests.gravity_enabled, is_gravity_enabled)
    is_awake = azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "IsAwake", sphere_id)
    Report.critical_result(Tests.start_asleep_enabled, not is_awake)

    # 6) Wait until timeout to check if sphere position changed
    general.idle_wait(TIME_OUT)

    # 7) Check the final x, y, z positions of the sphere
    final_sphere_pos = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", sphere_id)
    Report.critical_result(Tests.sphere_position_fixed, init_sphere_pos.Equal(final_sphere_pos))

    # 8) Exit game mode
    helper.exit_game_mode(Tests.exit_game_mode)



if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(RigidBody_StartAsleepWorks)
