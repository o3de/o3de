"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# Test case ID : C6090553
# Test Case Title : Check that force region exerts simple drag force on rigid bodies (negative test)



# fmt: off
class Tests():
    enter_game_mode          = ("Entered game mode",              "Failed to enter game mode")
    find_ball                = ("Ball found",                     "Ball not found")
    find_force_region        = ("Force Region found",             "Force Region not found")
    ball_gravity_disabled    = ("Ball gravity disabled",          "Ball gravity not disabled")
    ball_fell                = ("The ball fell",                  "The ball did not fall")
    ball_enters_force_region = ("Ball entered force region",      "Ball did not enter force region")
    ball_exits_force_region  = ("Ball exited force region",       "Ball did not exit force region")
    force_region_slows_ball  = ("Force Region did not slow ball", "Force Region slows ball")
    exit_game_mode           = ("Exited game mode",               "Couldn't exit game mode")
# fmt: on


def ForceRegion_ZeroSimpleDragForceDoesNothing():
    """
    Summary:
    Runs an automated test to ensure that force region exerts simple drag force on rigid bodies(negative test).

    Level Description:
    Ball (entity)         -  contains a sphere mesh, PhysX Collider (sphere shape) and PhysX RigidBody. Ball is
                             placed above force region
    Force Region (entity) -  contains Physx Force Region with Simple Drag force with Region Density as 0 and
                             PhysX Collider (box shape)

    Expected Behavior:
    When game mode is entered, Sphere falls through force region as though force region doesn't exist because
    region density for the simple drag force is zero and has no effect on the dropping sphere.

    Test Steps:
      1)  Open level
      2)  Enter Game mode
      3)  Validate the entities in the scene
      4)  Check ball gravity is disabled or not
      5)  Get initial velocity of ball
      6)  Wait for ball to enter force region
      7)  Gets z velocity and position of ball
      8)  Wait for ball to exit force region
      9)  Gets new velocity and position of ball
      10) Check that the ball does not slow due to the force region
      11) Exits game mode and editor

    Note:
    - This test file must be called from the Open 3D Engine Editor command terminal
    - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    import os
    import sys
    import azlmbr.legacy.general as general
    import azlmbr.bus

    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    # Holds details about the ball
    class Ball:
        id = None
        initial_velocity_z = 0.0
        start_velocity_z = 0.0
        end_velocity_z = 0.0
        ball_start_z_position = 0.0
        ball_end_z_position = 0.0
        entered_force_region = False
        exited_force_region = False
        ball_fell_down = False
        ball_slows = False

    def on_trigger_enter(args):
        other_id = args[0]
        if other_id.Equal(Ball.id):
            Ball.entered_force_region = True

    def on_trigger_exit(args):
        other_id = args[0]
        if other_id.Equal(Ball.id):
            Ball.exited_force_region = True

    # Constants
    TIMEOUT = 2.0
    CLOSE_ENOUGH_THRESHOLD = 0.01

    helper.init_idle()

    # 1) Open level
    helper.open_level("Physics", "ForceRegion_ZeroSimpleDragForceDoesNothing")

    # 2) Enter game mode
    helper.enter_game_mode(Tests.enter_game_mode)

    # 3) Validate the entities in the scene
    Ball.id = general.find_game_entity("Sphere")
    Report.critical_result(Tests.find_ball, Ball.id.IsValid())

    force_region_id = general.find_game_entity("Force Region")
    Report.critical_result(Tests.find_force_region, force_region_id.IsValid())

    # 4) Check ball gravity is disabled or not
    gravity_enabled = azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "IsGravityEnabled", Ball.id)
    Report.critical_result(Tests.ball_gravity_disabled, not gravity_enabled)

    # 5) Get initial velocity of ball
    # Ball linear velocity is set at (0, 0, -5) in the level
    Ball.initial_velocity_z = azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "GetLinearVelocity", Ball.id).z
    Report.info("Ball initial velocity = {}".format(Ball.initial_velocity_z))

    # 6) Wait for ball to enter force region
    handler = azlmbr.physics.TriggerNotificationBusHandler()
    handler.connect(force_region_id)
    handler.add_callback("OnTriggerEnter", on_trigger_enter)
    handler.add_callback("OnTriggerExit", on_trigger_exit)

    helper.wait_for_condition(lambda: Ball.entered_force_region, TIMEOUT)
    Report.critical_result(Tests.ball_enters_force_region, Ball.entered_force_region)

    # 7) Gets z velocity and position of ball
    Ball.start_velocity_z = azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "GetLinearVelocity", Ball.id).z
    Ball.ball_start_z_position = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldZ", Ball.id)
    Report.info(
        "Ball Start Z position = {}  Ball Start Z Velocity = {}".format(
            Ball.ball_start_z_position, Ball.start_velocity_z
        )
    )

    # 8) Wait for ball to exit force region
    helper.wait_for_condition(lambda: Ball.exited_force_region, TIMEOUT)
    Report.critical_result(Tests.ball_exits_force_region, Ball.exited_force_region)

    # 9) Gets new velocity and position of ball
    Ball.end_velocity_z = azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "GetLinearVelocity", Ball.id).z
    Ball.ball_end_z_position = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldZ", Ball.id)
    Report.info(
        "Ball End Z position = {}  Ball End Z Velocity = {}".format(Ball.ball_end_z_position, Ball.end_velocity_z)
    )

    # 10) Check that the ball does not slow due to the force region
    # Check ball fell down or not
    if (Ball.ball_end_z_position - CLOSE_ENOUGH_THRESHOLD) < Ball.ball_start_z_position:
        Ball.ball_fell_down = True

    Report.critical_result(Tests.ball_fell, Ball.ball_fell_down)

    # Ball initial velocity is -5.0. Check that the ball does not slow down in force region
    if ((Ball.end_velocity_z - Ball.initial_velocity_z) < CLOSE_ENOUGH_THRESHOLD) and (
        (Ball.start_velocity_z - Ball.initial_velocity_z) < CLOSE_ENOUGH_THRESHOLD
    ):
        Ball.ball_slows = True

    Report.critical_result(Tests.force_region_slows_ball, Ball.ball_slows)

    # 11) Exits game mode and editor
    helper.exit_game_mode(Tests.exit_game_mode)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(ForceRegion_ZeroSimpleDragForceDoesNothing)
