"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# Test case ID : C5959761
# Test Case Title : Check that force region (physics asset) exerts point force



# fmt: off
class Tests():
    enter_game_mode          = ("Entered game mode",         "Failed to enter game mode")
    find_ball                = ("Ball entity found",         "Ball entity not found")
    find_sedan               = ("Sedan entity found",        "Sedan entity not found")
    ball_fell                = ("The ball fell",             "The ball did not fall")
    ball_enters_force_region = ("Ball entered force region", "Ball did not enter force region")
    ball_exits_force_region  = ("Ball exited force region",  "Ball did not exit force region")
    ball_moved_up            = ("Ball moved up",             "Ball did not move up")
    ball_moved_forward       = ("Ball moved forward",        "Ball did not move forward")
    exit_game_mode           = ("Exited game mode",          "Couldn't exit game mode")
# fmt: on


def ForceRegion_PxMeshShapedForce():
    """
    Summary:
    Runs an automated test to ensure that PhysX force regions with physics assets exert point force on rigid bodies

    Level Description:
    A ball is suspended over a force region with a physics asset mesh (sedan). The ball is offset 1 unit towards the
    hood of the sedan (In the Y direction)

    Ball (entity) - Sphere shaped PhysX Collider; PhysX Rigid body with gravity enabled
    Sedan (entity) - Sedan shaped PhysX Collider; PhysX Force Region with a point force (magnitude 1000.0)

    Expected Behavior:
    The ball should fall once game mode is entered and bounce off the force region down and to the right.

    Test Steps:
    1) Open level
    2) Enter game mode
    3) Validate entities
    4) Wait for ball to fall
    5) Wait for ball to enter force region
    6) Wait for ball to exit force region
    7) Validate velocity vector
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
    import azlmbr
    import azlmbr.legacy.general as general
    import azlmbr.bus as bus
    import azlmbr.math

    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    TIMEOUT = 2.0

    class Ball:
        def __init__(self, name):
            self.id = general.find_game_entity(name)
            self.entered_force_region = False
            self.exited_force_region = False

        def get_velocity(self):
            return azlmbr.physics.RigidBodyRequestBus(bus.Event, "GetLinearVelocity", self.id)

        def is_falling(self):
            return self.get_velocity().z < 0.0

    helper.init_idle()
    # 1) Open level
    helper.open_level("Physics", "ForceRegion_PxMeshShapedForce")

    # 2) Enter game mode
    helper.enter_game_mode(Tests.enter_game_mode)

    # 3) Validate entities
    ball = Ball("Ball")
    Report.critical_result(Tests.find_ball, ball.id.IsValid())

    sedan_id = general.find_game_entity("Sedan")
    Report.critical_result(Tests.find_sedan, sedan_id.IsValid())

    # 4) Wait for ball to fall
    def on_trigger_enter(args):
        other_id = args[0]
        if other_id.Equal(ball.id):
            ball.entered_force_region = True

    def on_trigger_exit(args):
        other_id = args[0]
        if other_id.Equal(ball.id):
            ball.exited_force_region = True

    handler = azlmbr.physics.TriggerNotificationBusHandler()
    handler.connect(sedan_id)
    handler.add_callback("OnTriggerEnter", on_trigger_enter)
    handler.add_callback("OnTriggerExit", on_trigger_exit)

    Report.critical_result(Tests.ball_fell, helper.wait_for_condition(ball.is_falling, TIMEOUT))

    # 5) Wait for ball to enter force region
    helper.wait_for_condition(lambda: ball.entered_force_region, TIMEOUT)
    Report.critical_result(Tests.ball_enters_force_region, ball.entered_force_region)

    # 6) Wait for ball to exit force region
    helper.wait_for_condition(lambda: ball.exited_force_region, TIMEOUT)
    Report.critical_result(Tests.ball_exits_force_region, ball.exited_force_region)

    # 7) Validate velocity vector
    velocity = ball.get_velocity()
    Report.result(Tests.ball_moved_up, velocity.z > 0.0)
    Report.result(Tests.ball_moved_forward, velocity.y > 0.0)

    # 8) Exit game mode
    helper.exit_game_mode(Tests.exit_game_mode)



if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(ForceRegion_PxMeshShapedForce)
