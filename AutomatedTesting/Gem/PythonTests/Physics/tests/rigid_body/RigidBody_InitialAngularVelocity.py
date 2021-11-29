"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# Test case ID : C4976197
# Test Case Title : Verify that when you assign an Initial Angular Velocity to an object,
#   it moves with that Angular velocity when we switch to game mode


# fmt: off
class Tests:
    enter_game_mode       = ("Entered game mode",                 "Failed to enter game mode")
    gravity_disabled      = ("Gravity is disabled",               "Gravity is not disabled")
    cube_found            = ("Cube was found",                    "Cube was not found")
    trigger_found         = ("Trigger was found",                 "Trigger was not found")
    cube_pos              = ("Cube position is valid",            "Cube position is not valid")
    trigger_pos           = ("Trigger position is valid",         "Trigger position is not valid")
    cube_init_rotation    = ("Cube rotation is valid",            "Cube rotation is not valid")
    cube_touched_trigger  = ("Cube touched Trigger",              "Cube did not touch Trigger")
    cube_rotated_on_x     = ("Cube rotated on X axis",            "Cube did not rotate on X axis")
    cube_not_rotated_on_y = ("Cube did not rotate on Y axis",     "Cube rotated on Y axis")
    cube_not_rotated_on_z = ("Cube did not rotate on Z axis",     "Cube rotated on Z axis")
    cube_velocity         = ("The velocity is close to expected", "The velocity is not close to expected")
    exit_game_mode        = ("Exited game mode",                  "Couldn't exit game mode")
# fmt: on


def RigidBody_InitialAngularVelocity():
    # type: () -> None
    """
    Summary:
    Runs an automated test to ensure initial angular velocity of 5.0 rad/sec on X axis is exerted on a rigid body object

    Level Description:
    A cube positioned above terrain, with PhysX Shape Collider component with Box shape (dimensions: x=1, y=1, z=3),
    and with PhysX Rigid Body component, gravity disabled, initial angular velocity: (x=5, y=0, z=0) rad/s and
    angular damping set to 0.0
    A trigger with PhysX Shape Collider component with Box shape (dimensions: x=1, y=1, z=1), trigger enabled,
    positioned above terrain close to the Cube but not touching.

    Expected Behavior:
    When game mode is entered, the cube should rotate on x axis with 5 radian per second speed.
    The cube is supposed to touch the trigger as it rotates.

    Test Steps:
    1) Loads the level
    2) Enters game mode
    3) Retrieves test entities (Cube and Trigger)
    4) Checks if gravity is disabled
    5) Checks the Cube and Trigger locations
    6) Checks the Cube initial rotation
    7) Captures the angular velocity of cube
    8) Sets up Trigger
    9) Waits for the Cube to rotate and touch the Trigger
    10) Captures the Cube new rotation and reports results
    11) Exits game mode
    12) Closes the editor

    Note:
    - This test file must be called from the Open 3D Engine Editor command terminal
    - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: (None)
    """

    import os
    import sys
    import math



    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    import azlmbr.legacy.general as general
    import azlmbr.bus

    class Cube:
        id = None
        position = None
        init_rotation = None
        rotation = None
        angular_velocity = None
        touched_trigger = False
        # We found no bus to retrieve physics rigid body initial angular velocity
        # So we are hardcoding 5 as its initial angular velocity
        INITIAL_ANGULAR_VELOCITY = 5  # radian per second on X axis

    class Trigger:
        id = None
        position = None

    # Constants
    INITIAL_ANGULAR_VELOCITY_TOLERANCE = 0.05
    TIME_OUT = 2.0
    ROTATION_TOLERANCE = 0.001

    def is_angle_close(x, y):
        r = (math.sin(x) - math.sin(y)) * (math.sin(x) - math.sin(y)) + (math.cos(x) - math.cos(y)) * (
            math.cos(x) - math.cos(y)
        )
        diff = math.acos((2.0 - r) / 2.0)
        return abs(diff) <= ROTATION_TOLERANCE

    helper.init_idle()

    # 1) Load the level
    helper.open_level("Physics", "RigidBody_InitialAngularVelocity")

    # 2) Enter game mode
    helper.enter_game_mode(Tests.enter_game_mode)

    # 3) Retrieve entities
    Cube.id = general.find_game_entity("Cube")
    Trigger.id = general.find_game_entity("Trigger")
    Report.critical_result(Tests.cube_found, Cube.id.IsValid())
    Report.critical_result(Tests.trigger_found, Trigger.id.IsValid())

    # 4) Check gravity is disabled
    gravity_enabled = azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "IsGravityEnabled", Cube.id)
    Report.result(Tests.gravity_disabled, not gravity_enabled)

    # 5) Log Cube and Trigger positions
    Cube.position = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", Cube.id)
    valid_cube_pos = (Cube.position is not None) and (not Cube.position.IsZero())
    Report.info_vector3(Cube.position, "Cube initial position:")
    Report.critical_result(Tests.cube_pos, valid_cube_pos)

    Trigger.position = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", Trigger.id)
    valid_trigger_pos = (Trigger.position is not None) and (not Trigger.position.IsZero())
    Report.info_vector3(Trigger.position, "Trigger initial position:")
    Report.critical_result(Tests.trigger_pos, valid_trigger_pos)

    # 6) Log Cube initial rotation
    Cube.init_rotation = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldRotation", Cube.id)
    Report.info_vector3(Cube.init_rotation, "Cube initial rotation:")
    Report.critical_result(Tests.cube_init_rotation, (Cube.init_rotation is not None))

    # 7) Log Cube angular velocity
    Cube.angular_velocity = azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "GetAngularVelocity", Cube.id)
    Report.info_vector3(Cube.angular_velocity, "Cube angular velocity:")
    valid_velocity = abs(Cube.angular_velocity.x - Cube.INITIAL_ANGULAR_VELOCITY) < INITIAL_ANGULAR_VELOCITY_TOLERANCE
    Report.result(Tests.cube_velocity, valid_velocity)

    # 8) Set up Trigger
    def set_touched_value(args):
        entering_entity_id = args[0]
        if entering_entity_id.Equal(Cube.id):
            Report.info("Cube touched the Trigger")
            Cube.touched_trigger = True

    handler = azlmbr.physics.TriggerNotificationBusHandler()
    handler.connect(Trigger.id)
    handler.add_callback("OnTriggerEnter", set_touched_value)

    # 9) Wait a maximum of 2 seconds for the sphere to touch the trigger
    helper.wait_for_condition(lambda: Cube.touched_trigger, TIME_OUT)
    Report.result(Tests.cube_touched_trigger, Cube.touched_trigger)

    # 10) Log Cube's new rotation and validates it
    Cube.rotation = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldRotation", Cube.id)
    Report.info_vector3(Cube.rotation, "Cube new rotation:")
    Report.result(Tests.cube_rotated_on_x, not is_angle_close(Cube.rotation.x, Cube.init_rotation.x))
    Report.result(Tests.cube_not_rotated_on_y, is_angle_close(Cube.rotation.y, Cube.init_rotation.y))
    Report.result(Tests.cube_not_rotated_on_z, is_angle_close(Cube.rotation.z, Cube.init_rotation.z))

    # 11) Exit Game mode
    helper.exit_game_mode(Tests.exit_game_mode)



if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(RigidBody_InitialAngularVelocity)
