"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# Test case ID : C5932044
# Test Case Title : Check that force region exerts point force on rigid bodies



# fmt: off
class Tests():
    enter_game_mode             = ("Entered game mode",                                           "Failed to enter game mode"                                     )
    find_ball                   = ("Ball entity found",                                           "Ball entity not found"                                         )
    find_box                    = ("Box entity found",                                            "Box entity not found"                                          )
    gravity_works               = ("Ball fell",                                                   "Ball did not fall"                                             )
    ball_entered_force_region   = ("Ball entered force region",                                   "Ball did not enter force region before timeout"                )
    ball_exited_force_region    = ("Ball exited force region",                                    "Ball did not exit force region before timeout"                 )
    net_force_magnitude         = ("The net force magnitude on ball is close to expected value",  "The net force magnitude on ball is not close to expected value")
    ball_moved_up               = ("Ball moved up",                                               "Ball did not move up"                                          )
    ball_moved_right            = ("Ball moved right",                                            "Ball did not move right"                                       )
    ball_not_moved_y            = ("Ball did not move in the y direction",                        "Ball moved in the y direction"                                 )
    exit_game_mode              = ("Exited game mode",                                            "Couldn't exit game mode"                                       )
# fmt: on


def ForceRegion_PointForceOnRigidBodies():
    """
    Summary:
    Runs an automated test to ensure that that a force region exerts point force on rigid bodies

    Level Description:
    RigidBody (entity) - a sphere suspended above and to the right the a force region with gravity enabled;
        contains a sphere mesh, PhysX Collider (sphere shape), and PhysX RigidBody
    ForceRegion (entity) - contains box mesh, PhysX Collider (Box shape), and PhysX RigidBody

    Expected Behavior:
    When game mode is entered, the ball entity will experience gravity and fall toward the upper right edge (+z, +x) of
    the force region. The force region applies a point force to the ball, sending it upwards (+z) and to the right (+x)

    Test Steps:
     1) Open level
     2) Enter game mode
     3) Retrieve entities
     4) Get the starting x & z position of the ball
     5) Check that the ball falls (gravity check)
     6) Check that the ball enters the trigger area
     7) Get the magnitude of the collision
     8) Check that the ball exits the trigger area
     9) Verify that the magnitude of the collision is as expected
    10) Check that the ball is moving up and to the right
    11) Exit game mode
    12) Close the editor

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

    class Ball:
        start_position_x = None
        start_position_z = None
        fell = False

    # Constants
    TIMEOUT = 2.0
    MAGNITUDE_TOLERANCE = 0.2                      # Magnitudes must be within this amount in order to be valid
    NO_MOTION_Y_TOLERANCE = sys.float_info.epsilon # Motion in the y axis must be below this in order to be valid

    helper.init_idle()
    # 1) Open level
    helper.open_level("Physics", "ForceRegion_PointForceOnRigidBodies")

    # 2) Enter game mode
    helper.enter_game_mode(Tests.enter_game_mode)

    # 3) Retrieve entities
    ball_id = general.find_game_entity("RigidBody")
    Report.critical_result(Tests.find_ball, ball_id.IsValid())

    box_id = general.find_game_entity("ForceRegion")
    Report.critical_result(Tests.find_box, box_id.IsValid())

    # 4) Get the starting x & z position of the ball
    Ball.start_position_x = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldX", ball_id)
    Ball.start_position_z = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldZ", ball_id)
    Report.info("Ball start X: {}".format(Ball.start_position_x))
    Report.info("Ball start Z: {}".format(Ball.start_position_z))

    # 5) Check that the ball falls (gravity check)
    def ball_falls():
        if not Ball.fell:
            ball_position_z = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldZ", ball_id)
            if (ball_position_z - Ball.start_position_z) < 0.0:
                Report.info("Ball position is now lower than the starting position")
                Ball.fell = True
        return Ball.fell

    helper.wait_for_condition(ball_falls, TIMEOUT)
    Report.result(Tests.gravity_works, Ball.fell)

    # 6) Check that the ball enters the trigger area
    class ForceRegionTrigger:
        entered = False
        exited = False

    def on_enter(args):
        other_id = args[0]
        if other_id.Equal(ball_id):
            Report.info("Trigger entered")
            ForceRegionTrigger.entered = True

    def on_exit(args):
        other_id = args[0]
        if other_id.Equal(ball_id):
            Report.info("Trigger exited")
            ForceRegionTrigger.exited = True

    handler = azlmbr.physics.TriggerNotificationBusHandler()
    handler.connect(box_id)
    handler.add_callback("OnTriggerEnter", on_enter)
    handler.add_callback("OnTriggerExit", on_exit)

    helper.wait_for_condition(lambda: ForceRegionTrigger.entered, TIMEOUT)
    Report.result(Tests.ball_entered_force_region, ForceRegionTrigger.entered)

    # 7) Get the magnitude of the collision
    class NetForceMagnitude:
        value = 0

    def on_calc_net_force(args):
        """
        args[0] - force region entity
        args[1] - entity entering
        args[2] - vector
        args[3] - magnitude
        """
        force_magnitude = args[3]
        NetForceMagnitude.value = force_magnitude

    force_notification_handler = azlmbr.physics.ForceRegionNotificationBusHandler()
    force_notification_handler.connect(None)
    force_notification_handler.add_callback("OnCalculateNetForce", on_calc_net_force)

    # 8) Check that the ball exits the trigger area
    helper.wait_for_condition(lambda: ForceRegionTrigger.exited, TIMEOUT)
    Report.result(Tests.ball_exited_force_region, ForceRegionTrigger.exited)

    # 9) Verify that the magnitude of the collision is as expected
    def is_close_float(a, b, tolerance):
        return abs(b - a) < tolerance

    force_region_magnitude = azlmbr.physics.ForcePointRequestBus(azlmbr.bus.Event, "GetMagnitude", box_id)
    Report.info(
        "NetForce magnitude is {}, Force Region magnitude is {}".format(NetForceMagnitude.value, force_region_magnitude)
    )
    Report.result(
        Tests.net_force_magnitude, is_close_float(NetForceMagnitude.value, force_region_magnitude, MAGNITUDE_TOLERANCE)
    )

    # 10) Check that the ball is moving up and to the right
    linear_velocity = azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "GetLinearVelocity", ball_id)
    Report.info_vector3(linear_velocity, "Ball linear velocity")
    Report.result(Tests.ball_moved_up, linear_velocity.z > 0)
    Report.result(Tests.ball_moved_right, linear_velocity.x > 0)
    Report.result(Tests.ball_not_moved_y, abs(linear_velocity.y) < NO_MOTION_Y_TOLERANCE)

    # 11) Exit game mode
    helper.exit_game_mode(Tests.exit_game_mode)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(ForceRegion_PointForceOnRigidBodies)
