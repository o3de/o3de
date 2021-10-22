"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# Test case ID : C15556261
# Test Case Title : Check that the material assignment works with Character Controller



# fmt: off
class Tests:

    # level
    enter_game_mode           = ("Entered game mode",                        "Failed to enter game mode")
    exit_game_mode            = ("Exited game mode",                         "Couldn't exit game mode")

    #balls
    ball_to_hit_rubber_char_controller_found     = ("ball_to_hit_rubber_char_controller found",                    "ball_to_hit_rubber_char_controller NOT FOUND ")
    ball_to_hit_rubber_char_controller_gravity   = ("ball_to_hit_rubber_char_controller gravity is disabled",      "ball_to_hit_rubber_char_controller GRAVITY IS ENABLED ")
    ball_to_hit_rubber_char_controller_position  = ("ball_to_hit_rubber_char_controller valid postion",            "ball_to_hit_rubber_char_controller INVALID POSITION ")
    ball_to_hit_rubber_char_controller_collision = ("ball_to_hit_rubber_char_controller collided with its target", "ball_to_hit_rubber_char_controller DID NOT COLLIDE WITH its target")
    
    ball_to_hit_glass_char_controller_found      = ("ball_to_hit_glass_char_controller found",                     "ball_to_hit_glass_char_controller NOT FOUND ")
    ball_to_hit_glass_char_controller_gravity    = ("ball_to_hit_glass_char_controller gravity is disabled",       "ball_to_hit_glass_char_controller GRAVITY IS ENABLED ")
    ball_to_hit_glass_char_controller_position   = ("ball_to_hit_glass_char_controller valid postion",             "ball_to_hit_glass_char_controller INVALID POSITION ")
    ball_to_hit_glass_char_controller_collision  = ("ball_to_hit_glass_char_controller collided with its target",  "ball_to_hit_glass_char_controller DID NOT COLLIDE WITH its target")
    
    ball_to_hit_rock_char_controller_found       = ("ball_to_hit_rock_char_controller found",                      "ball_to_hit_rock_char_controller NOT FOUND ")
    ball_to_hit_rock_char_controller_gravity     = ("ball_to_hit_rock_char_controller gravity is disabled",        "ball_to_hit_rock_char_controller GRAVITY IS ENABLED ")
    ball_to_hit_rock_char_controller_position    = ("ball_to_hit_rock_char_controller valid postion",              "ball_to_hit_rock_char_controller INVALID POSITION ")
    ball_to_hit_rock_char_controller_collision   = ("ball_to_hit_rock_char_controller collided with its target",   "ball_to_hit_rock_char_controller DID NOT COLLIDE WITH its target")

    # targets
    char_rubber_found                            = ("character controller rubber found",                           "character controller rubber NOT FOUND ")
    char_rock_found                              = ("character controller rock found",                             "character controller rock NOT FOUND ")
    char_glass_found                             = ("character controller glass found",                            "character controller glass NOT FOUND ")

    # balls velocity
    balls_velocity                               = ("balls velocity : rubber > glass > rock",                      "unexpected balls velocity")
# fmt: on


def Material_CharacterController():
    """
    Summary:
    Runs an automated test to verify that character controllers with different surface materials behave accordingly.

    Level Description:
    3 character controllers with capsule shape, surface materials: rubber, rock, glass.
    3 balls with sphere shape on same X and Z coordinates of each character controller, initial linear velocity
    of 5 m/s on Y axis. All 3 balls have rock surface material.

    Expected Behavior:
    The balls should all hit their corresponding character controller.
    The character controller with rubber should make the ball bounce back with almost the same speed.
    The one with glass should make the ball bounce but with reduced speed.
    The ball should not bounce off the character controller with rock material.
    The balls linear velocity is checked at the end of the test. Expected results for linear velocities are:
    rubber > glass > rock

    Test Steps:
    1) Loads the level
    2) Enters game mode
    3) Setup balls
        3.1) Validate ball ID
        3.2) Validate ball gravity
        3.3) Connect ball to target
        3.4) Validate ball position
    4) Wait for balls to collide
    5) Get balls velocity
    6) Validate velocity is as rubber > glass > rock
    7) Exit game mode
    8) Close editor
    """

    import os
    import sys
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper
    import azlmbr.legacy.general as general
    import azlmbr.bus

    # Constants
    TIME_OUT = 3.0
    WAIT_TIME_AFTER_COLLISSION = 0.1

    def is_close(value1, value2, tolerance=0.01):
        return abs(value1 - value2) <= tolerance

    def get_test(entity_name, suffix):
        return Tests.__dict__[entity_name + suffix]

    class Entity:  # Base class for targets and balls
        def __init__(self, name):
            self.name = name
            self.id = None
            self.position = None
            self.gravity = None

        def validate_ID(self):
            self.id = general.find_game_entity(self.name)
            found_tuple = get_test(self.name, "_found")
            Report.critical_result(found_tuple, self.id.IsValid())

    class CharacterController(Entity):
        def __init__(self, name):
            Entity.__init__(self, name)
            self.material = self.name.rpartition("_")[2]

    class Ball(Entity):
        def __init__(self, name, target_name):
            Entity.__init__(self, name)
            self.target_name = target_name
            self.entered_times = 0
            self.collided_with_target = False
            # 3.1) Validate ball ID
            self.validate_ID()
            # 3.2) Validate gravity is disabled
            self.validate_gravity()
            # 3.3) Setup collision targets
            self.setup_target()
            # 3.4) Validate ball position
            self.validate_position()

        def validate_position(self):
            self.position = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", self.id)
            self.target.position = azlmbr.components.TransformBus(
                azlmbr.bus.Event, "GetWorldTranslation", self.target.id
            )
            position_tuple = get_test(self.name, "_position")
            Report.critical_result(
                position_tuple,
                (is_close(self.position.x, self.target.position.x))
                and (is_close(self.position.z, self.target.position.z + 1)),
            )

        def validate_gravity(self):
            gravity_enabled = azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "IsGravityEnabled", self.id)
            gravity_tuple = get_test(self.name, "_gravity")
            Report.critical_result(gravity_tuple, not gravity_enabled)

        def detect_collision_target(self, args):
            entering_entity_id = args[0]
            if entering_entity_id.Equal(self.target.id):
                Report.info(self.name + " collided with " + self.target.name)
                self.collided_with_target = True
                collision_tuple = get_test(self.name, "_collision")
                Report.critical_result(collision_tuple, self.collided_with_target)

        def setup_target(self):
            self.target = CharacterController(self.target_name)
            self.target.validate_ID()
            self.collision_handler = azlmbr.physics.CollisionNotificationBusHandler()
            self.collision_handler.connect(self.id)
            self.collision_handler.add_callback("OnCollisionBegin", self.detect_collision_target)

    helper.init_idle()

    # 1) Load the level
    helper.open_level("Physics", "Material_CharacterController")

    # 2) Enter game mode
    helper.enter_game_mode(Tests.enter_game_mode)

    # 3) Setup balls
    all_balls = [
        Ball(name="ball_to_hit_rubber_char_controller", target_name="char_rubber"),
        Ball(name="ball_to_hit_glass_char_controller", target_name="char_glass"),
        Ball(name="ball_to_hit_rock_char_controller", target_name="char_rock"),
    ]

    # 4) Wait for balls movement
    helper.wait_for_condition(lambda: all(ball.collided_with_target for ball in all_balls), TIME_OUT)
    general.idle_wait(WAIT_TIME_AFTER_COLLISSION)

    # 5) Get each ball's linear velocity after collision
    for ball in all_balls:
        ball.linear_velocity_magnitude = azlmbr.physics.RigidBodyRequestBus(
            azlmbr.bus.Event, "GetLinearVelocity", ball.id
        ).GetLength()

    # 6) Check ball's velocity
    Report.result(
        Tests.balls_velocity,
        all_balls[0].linear_velocity_magnitude
        > all_balls[1].linear_velocity_magnitude
        > all_balls[2].linear_velocity_magnitude,
    )

    # 7) Exit Game mode
    helper.exit_game_mode(Tests.exit_game_mode)



if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Material_CharacterController)
