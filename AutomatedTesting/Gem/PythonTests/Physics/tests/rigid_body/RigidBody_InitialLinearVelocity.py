"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# Test case ID : C4976195
# Test Case Title : Verify that when you assign an Initial Linear Velocity to an object,
# ... it moves with that linear velocity when we switch to game mode.



# fmt: off
class Tests:
    enter_game_mode = ("Entered game mode",                       "Failed to enter game mode")
    entities_found  = ("Entities found",                          "Entities not found")
    entities_pos    = ("Entities positions are valid",            "Entities positions are not valid")
    sphere_moved    = ("Sphere moved correctly",                  "Sphere did not move correctly")
    sphere_velocity = ("The magnitude of velocity is close to 5", "The magnitude of velocity is not close to 5")
    exit_game_mode  = ("Exited game mode",                        "Couldn't exit game mode")

def RigidBody_InitialLinearVelocity():
    # type: () -> None
    """
    Summary:
    Runs an automated test to ensure intial linear velocity of 5.0 on X axis is exerted on a rigid body object.

    Level Description:
    A sphere (entity: Sphere) positioned above terrain which is assigned with
    an initial linear velocity of 5.0 in x direction. The Sphere has gravity disabled.
    A trigger cube positioned on the same Y and Z but different X of the sphere.

    Expected Behavior:
    When game mode is entered, the sphere should move on x direction with 5 m/s speed.
    The sphere is supposed to touch the trigger cube as it moves.

    Test Steps:
    1) Loads the level
    2) Enters game mode
    3) Retrieves test entities (Sphere and Trigger Cube)
    4) Ensures that the sphere and the cube are located
    5) Captures the velocity of sphere
        5.1) Sets up trigger cube
    6) Waits for the shpere to move and touch the trigger cube
    7) Captures the sphere new location
    8) Exits game mode
    9) Closes the editor

    Note:
    - This test file must be called from the Open 3D Engine Editor command terminal
    - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: (None)
    """

    import os
    import sys
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    import azlmbr.legacy.general as general
    import azlmbr.bus

    helper.init_idle()

    # 1) Load the level
    helper.open_level("Physics", "RigidBody_InitialLinearVelocity")

    # 2) Enter game mode
    helper.enter_game_mode(Tests.enter_game_mode)

    # 3) Retrieve entities
    sphere_id = general.find_game_entity("Sphere")
    trigger_cube_id = general.find_game_entity("TriggerCube")
    valid_entity_ids = (sphere_id.IsValid() and trigger_cube_id.IsValid())
    Report.critical_result(Tests.entities_found, valid_entity_ids)

    # 4) Log sphere and cube initial position
    init_sphere_pos = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", sphere_id)
    trigger_cube_pos = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", trigger_cube_id)
    sphere_pos_found = ((init_sphere_pos is not None) and (not init_sphere_pos.IsZero()))
    trigger_cube_pos_found = ((trigger_cube_pos is not None) and (not trigger_cube_pos.IsZero()))
    pos_found = sphere_pos_found and trigger_cube_pos_found
    Report.critical_result(Tests.entities_pos, pos_found)
    Report.info_vector3(init_sphere_pos, "Sphere initial position:")

    # 5) Log Sphere's velocity
    sphere_linear_velocity = azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "GetLinearVelocity", sphere_id)
    Report.info("Sphere linear velocity {}".format(sphere_linear_velocity.ToString()))
    sphere_linear_velocity_magnitude = sphere_linear_velocity.GetLength()
    Report.info("Magnitude = {}".format(sphere_linear_velocity_magnitude))
    # We found no bus to retrieve physics rigid body initial linear velocity
    # So we are hardcoding 5 as its initial linear velocity
    SPHERE_INITIAL_VELOCITY = 5
    outcome = abs(sphere_linear_velocity_magnitude - SPHERE_INITIAL_VELOCITY) < 0.05
    Report.result(Tests.sphere_velocity, outcome)

    # 5.1) Set up trigger cube
    class SphereTouchedTrigger:
        value = False

    def set_touched_value(args):
        # Called when the sphere touches the trigger cube
        SphereTouchedTrigger.value = True

    handler = azlmbr.physics.TriggerNotificationBusHandler()
    handler.connect(trigger_cube_id)
    handler.add_callback("OnTriggerEnter", set_touched_value)

    # 6) Wait a maximum of 3 seconds for the sphere to touch the trigger
    helper.wait_for_condition(lambda: SphereTouchedTrigger.value, 3.0)

    # 7) Log Sphere's new location and validates sphere's movement
    new_sphere_pos = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", sphere_id)
    # The sphere is supposed to move on X axis, but not on Y nor Z
    valid_movement = (new_sphere_pos.x > init_sphere_pos.x and
                      abs(new_sphere_pos.y - init_sphere_pos.y) < 0.001 and
                      abs(new_sphere_pos.z - init_sphere_pos.z) < 0.001)
    Report.result(Tests.sphere_moved, valid_movement)
    Report.info_vector3(new_sphere_pos, "Sphere new position:")

    # 8) Exit Game mode
    helper.exit_game_mode(Tests.exit_game_mode)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(RigidBody_InitialLinearVelocity)
