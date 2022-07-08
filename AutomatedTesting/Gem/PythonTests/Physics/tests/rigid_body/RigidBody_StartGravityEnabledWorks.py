"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# Test case ID : C4976206
# Test Case Title : Verify that when Gravity enables is checked, the object falls down due to gravity [sic]


# fmt: off
class Tests:
    enter_game_mode             = ("Entered game mode",         "Failed to enter game mode")
    find_ball                   = ("Ball entity found",         "Ball entity not found")
    find_terrain                = ("Terrain entity found",      "Terrain entity not found")
    gravity_started_disabled    = ("Gravity started disabled",  "Gravity didn't start disabled")
    gravity_set_enabled         = ("Gravity has been enabled",  "Gravity wasn't enabled")
    ball_fell                   = ("Ball fell",                 "Ball didn't fall")
    touched_ground              = ("Ball touched the ground",   "Ball did not touch the ground")
    exit_game_mode              = ("Exited game mode",          "Couldn't exit game mode")
# fmt: on


# Wait times defined in frames for waiting while in game mode
class Frames:
    entity_load = 2
    delta_frames = 1


def RigidBody_StartGravityEnabledWorks():
    """
    Summary:
    Runs automated test to verify that when the gravity enabled checkbox is ticked,
    the object will fall down due to gravity.

    Level Description:
    A ball entity is suspended in the air with gravity disabled by default
    Below the ball is a terrain entity with a PhysX Terrain component

    Expected Behavior:
    The level opens and by default gravity is not enabled on the ball.
    When game mode is entered, the ball should not fall until gravity is enabled,
    then the ball should fall and collide with the terrain.

    Test Steps:
     1) Open level and enter game mode
     2) Find the entities
     3) Make sure that gravity is turned off by default
     4) Get the Z position before enabling gravity
     5) Activate gravity
     6) Check that the ball is falling towards the terrain
     7) Check that there is a collision with the PhysX Terrain
     8) Exit game mode and close the editor

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

    timeout_seconds = 3.0

    helper.init_idle()

    # 1) Open level and enter game mode
    helper.open_level("Physics", "RigidBody_StartGravityEnabledWorks")
    helper.enter_game_mode(Tests.enter_game_mode)

    # 2) Retrieve entities
    class Ball:
        id = None
        gravity_enabled = None
        starting_height = None
        current_height = None
        fell = False
        touched_ground = False

    general.idle_wait_frames(Frames.entity_load)

    ball_id = general.find_game_entity("RigidBody")
    Ball.id = ball_id
    Report.critical_result(Tests.find_ball, Ball.id.IsValid())

    terrain_id = general.find_game_entity("Terrain")
    Report.critical_result(Tests.find_terrain, terrain_id.IsValid())

    # 3) Make sure gravity is off from the start
    Ball.gravity_enabled = azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "IsGravityEnabled", Ball.id)
    Report.result(Tests.gravity_started_disabled, not Ball.gravity_enabled)

    # 4) Get the Z position before enabling the physics
    Ball.starting_height = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldZ", Ball.id)

    # 5) Activate gravity
    general.idle_wait_frames(Frames.delta_frames)
    Report.info("Enabling Gravity")

    azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "SetGravityEnabled", Ball.id, True)

    Ball.gravity_enabled = azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "IsGravityEnabled", Ball.id)
    Report.critical_result(Tests.gravity_set_enabled, Ball.gravity_enabled)

    # 6) Compare the Z position after enabling the gravity and check that there is a collision with the PhysX Terrain
    def ball_moved_down():
        Ball.current_height = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldZ", Ball.id)
        if Ball.current_height < (Ball.starting_height - 1):
            Ball.fell = True
            return True
        return False

    def on_collision_begin(args):
        other_id = args[0]
        if other_id.Equal(terrain_id):
            Report.info("Touched ground")
            Ball.touched_ground = True

    handler = azlmbr.physics.CollisionNotificationBusHandler()
    handler.connect(Ball.id)
    handler.add_callback("OnCollisionBegin", on_collision_begin)

    helper.wait_for_condition(lambda : (ball_moved_down() and Ball.touched_ground), timeout_seconds)
    Report.info("Ball.start_height: {} Ball.current_height: {}".format(Ball.starting_height, Ball.current_height))
    Report.result(Tests.ball_fell, Ball.fell)
    Report.result(Tests.touched_ground, Ball.touched_ground)

    # 8) Exit game mode and close the editor
    helper.exit_game_mode(Tests.exit_game_mode)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(RigidBody_StartGravityEnabledWorks)
