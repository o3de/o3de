"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# Test case ID : C5932040
# Test Case Title : Check that force region exerts world space force on rigid bodies



# fmt: off
class Tests():
    enter_game_mode             = ("Entered game mode",                                              "Failed to enter game mode")
    find_ball                   = ("Ball entity found",                                              "Ball entity not found")
    find_box                    = ("Box entity found",                                               "Box entity not found")
    gravity_works               = ("Ball fell",                                                      "Ball did not fall")
    ball_triggers_force_region  = ("Ball triggered force region",                                    "Ball did not trigger force region")
    net_force_magnitude         = ("The net force magnitude on the ball is close to expected value", "The net force magnitude on the ball is not close to expected value")
    ball_moved_up               = ("Ball moved up",                                                  "Ball did not move up before timeout occurred")
    exit_game_mode              = ("Exited game mode",                                               "Couldn't exit game mode")
# fmt: on


def is_close_float(a, b, factor):
    return abs(b - a) < factor


def ForceRegion_WorldSpaceForceOnRigidBodies():
    """
    Summary:
    A ball is suspended over a box shaped force region.
    The ball drops and is forced upward by the world space force of the force region

    Level Description:
    Ball (entity) - Sphere shaped Mesh; Sphere shaped PhysX Collider; PhysX Rigid Body
    ForceRegion (entity) - Cube shaped Mesh; Cube shaped PhysX Collider; PhysX Force Region with world space force

    Expected Behavior:
    The level opens and enters game mode. At this time the ball will fall towards the force region.
    When it collides, it will be launched upwards. Then the game mode will exit and the editor will close.

    Test Steps:
     1) Open level
     2) Enter game mode
     3) Find the entities
     4) Get starting position of the ball
     5) Check that gravity works and ball falls
     6) Check that the ball enters the trigger area of force region
     7) Get the magnitude of the collision
     8) Check that the ball moved up
     9) Verify that the magnitude of the collision is as expected
    10) Exit game mode
    11) Close the editor

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
        start_position_z = None
        fell = False

    # Constants
    TIMEOUT = 2.0
    BALL_MIN_MOVED_UP = 5  # Minimum amount to indicate Z movement
    MAGNITUDE_TOLERANCE = 0.26  # Force region magnitude tolerance

    helper.init_idle()
    # 1) Open level
    helper.open_level("Physics", "ForceRegion_WorldSpaceForceOnRigidBodies")

    # 2) Enter game mode
    helper.enter_game_mode(Tests.enter_game_mode)

    # 3) Retrieve entities
    ball_id = general.find_game_entity("Ball")
    Report.critical_result(Tests.find_ball, ball_id.IsValid())

    box_id = general.find_game_entity("ForceRegion")
    Report.critical_result(Tests.find_box, box_id.IsValid())

    # 4) Get the starting z position of the ball
    Ball.start_position_z = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldZ", ball_id)
    Report.info("Starting Height of the ball: {}".format(Ball.start_position_z))

    # 5) Check that gravity works and the ball falls
    def ball_falls():
        if not Ball.fell:
            ball_after_frame_z = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldZ", ball_id)
            if (ball_after_frame_z - Ball.start_position_z) < 0.0:
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
    Report.result(Tests.ball_triggers_force_region, ForceRegionTrigger.entered)

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

    def ball_moved_up():
        ball_z = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldZ", ball_id)
        return ball_z > Ball.start_position_z + BALL_MIN_MOVED_UP

    # 8) Check that the ball moved up
    if helper.wait_for_condition(lambda: ball_moved_up and ForceRegionTrigger.exited, TIMEOUT):
        Report.success(Tests.ball_moved_up)
    else:
        Report.failure(Tests.ball_moved_up)

    # 9) Verify that the magnitude of the collision is as expected
    force_region_magnitude = azlmbr.physics.ForceWorldSpaceRequestBus(azlmbr.bus.Event, "GetMagnitude", box_id)
    Report.info(
        "NetForce magnitude is {}, Force Region magnitude is {}".format(NetForceMagnitude.value, force_region_magnitude)
    )
    Report.result(
        Tests.net_force_magnitude, is_close_float(NetForceMagnitude.value, force_region_magnitude, MAGNITUDE_TOLERANCE)
    )

    # 10) Exit game mode
    helper.exit_game_mode(Tests.exit_game_mode)



if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(ForceRegion_WorldSpaceForceOnRigidBodies)
