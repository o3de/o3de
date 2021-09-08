"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# Test case ID : C4925577
# Test Case Title : Verify that material can be assigned to PhysX terrain in Terrain Texture Layers


# fmt: off
class Tests:
    game_mode_enter             = ("Game mode was successfully entered",            "Game mode could not be entered")
    find_terrain                = ("Terrain was found",                             "Terrain was not found")
    find_ball_default           = ("Ball_Default was found",                        "Ball_Default was not found")
    find_ball_rubber            = ("Ball_Rubber was found",                         "Ball_Rubber was not found")
    find_ball_concrete          = ("Ball_Concrete was found",                       "Ball_Concrete was not found")
    all_gravity_disabled        = ("All the balls started with gravity disabled",   "Not all the balls started with gravity disabled")
    same_starting_height        = ("The 3 balls started at the same height",        "The 3 balls were not the same height at start")
    balls_are_aligned           = ("The balls are initially lined up properly",     "The balls are not initially lined up properly")
    terrain_collide_default     = ("Ball_Default has collided with terrain",        "Ball_Default timed out before colliding with terrain")
    terrain_collide_rubber      = ("Ball_Rubber has collided with terrain",         "Ball_Rubber timed out before colliding with terrain")
    terrain_collide_concrete    = ("Ball_Concrete has collided with terrain",       "Ball_Concrete timed out before colliding with terrain")
    peak_reached_default        = ("Ball_Default has reached peak height",          "Ball_Default timed out before reaching peak")
    peak_reached_rubber         = ("Ball_Rubber has reached peak height",           "Ball_Rubber timed out before reaching peak")
    peak_reached_concrete       = ("Ball_Concrete has reached peak height",         "Ball_Concrete timed out before reaching peak")
    bounce_height_order_correct = ("The ball bounce heights are correctly ordered", "The ball bounce heights are not correctly ordered")
    game_mode_exit              = ("Game mode was successfully exited",             "Game mode could not exit properly")
# fmt: on


def Material_CanBeAssignedToTerrain():
    """
    Summary:
    Three spheres are suspended above the terrain. Beneath two of the balls,
    there is a different material painted on the terrain.
    They should all bounce at different heights per their respective terrains

    Terrain entity: PhysX Terrain component: default settings
    Ball Entities:  Sphere shaped Mesh component
                    Sphere shaped PhysX Collider component: default settings
                    PhysX Rigid Body component: Gravity disabled, default settings

    Concrete Material: Restitution: 0.0; Restitution Combine: Average
    Rubber Material:   Restitution: 1.0; Restitution Combine: Average

    Expected Behavior:
    The three balls start off at the same height. When game mode is entered they will fall towards the terrain.
    After the ball collides with the terrain, they will bounce back at different heights respective to their
    terrain material collisions. Ball_Default is the control and is dropped on default terrain material.
    Ball_Rubber bounces off the rubber terrain material and should bounce higher than the default.
    Ball_Concrete strikes the concrete terrain material and should not bounce as high as the default material.

    Test Steps:
     1) Open level
     2) Enter game mode
     3) Find entities
     4) Check that gravity is disabled for all the balls initially
     5) Check that the balls are aligned and all falling from the same height
        Steps 6-9 run for each ball
         6) Assign the tests and enable handlers to their respective spheres
         7) Enable gravity on ball entities
         8) Check that the balls collide with the PhysX Terrain
         9) Wait for the ball to reach its peak height; record height and freeze it
    10) Compare the bounce heights of the balls
    11) Exit game mode and close the editor

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
    import azlmbr.physics as phys
    import azlmbr.math as mathazon

    # fmt: off
    ZERO_VECTOR = mathazon.Vector3(0.0, 0.0, 0.0)
    X_POSITION_RUBBER = 60.0        # Point on X axis material was painted rubber during level setup
    X_POSITION_DEFAULT = 70.0       # Area in between other materials where Default material exists
    X_POSITION_CONCRETE = 80.0      # Point on X axis material was painted concrete during level setup
    Y_POSITION_VALUE = 42.0         # Point on Y axis materials were painted during level setup
    POSITION_BUFFER = 4.0           # Material paint radius is 4.0 m
    TIMEOUT_IN_SECONDS = 3.0
    NUM_WAIT_FRAMES_ENTITY_LOAD = 2 # Frames to wait to allow entities to load in level
    # fmt: on

    class Terrain:
        id = None
        name = None
        handler = None

    class Sphere:
        def __init__(self, name):
            self.name = name
            self.id = general.find_game_entity(self.name)
            self.gravity_enabled = phys.RigidBodyRequestBus(azlmbr.bus.Event, "IsGravityEnabled", self.id)
            self.world_location_start = self.get_location()
            self.handler = None
            self.hit_ground = False
            self.bounced = False
            self.peak_reached = False
            self.ground_height = 0.0
            self.peak_height = 0.0

        def assign_tests(self):
            if self.name == "Ball_Default":
                self.test_find_ball = Tests.find_ball_default
                self.test_terrain_collide = Tests.terrain_collide_default
                self.test_peak_reached = Tests.peak_reached_default

            elif self.name == "Ball_Rubber":
                self.test_find_ball = Tests.find_ball_rubber
                self.test_terrain_collide = Tests.terrain_collide_rubber
                self.test_peak_reached = Tests.peak_reached_rubber

            elif self.name == "Ball_Concrete":
                self.test_find_ball = Tests.find_ball_concrete
                self.test_terrain_collide = Tests.terrain_collide_concrete
                self.test_peak_reached = Tests.peak_reached_concrete

        def get_location(self):
            # () -> Vector3
            return azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", self.id)

        def get_linear_velocity(self):
            # () -> Vector3
            return phys.RigidBodyRequestBus(azlmbr.bus.Event, "GetLinearVelocity", self.id)

        def set_linear_velocity(self, vector):
            # (Vector3) -> None
            phys.RigidBodyRequestBus(azlmbr.bus.Event, "SetLinearVelocity", self.id, vector)

        def freeze_self(self):
            # () -> None
            self.set_linear_velocity(ZERO_VECTOR)
            self.enable_gravity(False)

        def check_gravity(self):
            # () -> bool
            return phys.RigidBodyRequestBus(azlmbr.bus.Event, "IsGravityEnabled", self.id)

        def enable_gravity(self, bool_to_set=True):
            # (bool) -> None
            phys.RigidBodyRequestBus(azlmbr.bus.Event, "SetGravityEnabled", self.id, bool_to_set)

        def peak_height_reached(self):
            """
            Used for conditional waiting;
            If peak is reached: sets the value for self.peak_reached to True, saves peak world height,
                freezes self to keep it from continuing to bounce and possibly interfering with another ball instance
            """
            current_location = self.get_location()
            if current_location.z < self.peak_height:
                self.peak_reached = True
                Report.info("{} has peaked at {:.6} in the world.".format(self.name, self.peak_height))
                self.freeze_self()
                return True
            self.peak_height = current_location.z
            return False

        def on_collision_begin(self, args):
            # Ball collides with the ground
            other_id = args[0]
            other_name = azlmbr.entity.GameEntityContextRequestBus(azlmbr.bus.Broadcast, "GetEntityName", other_id)
            if other_name == Terrain.name:
                self.hit_ground = True
                Report.info("{} has collided with the terrain.".format(self.name))
                location = self.get_location()
                self.ground_height = location.z

        def on_collision_end(self, args):
            # Ball bounces off the ground
            other_id = args[0]
            other_name = azlmbr.entity.GameEntityContextRequestBus(azlmbr.bus.Broadcast, "GetEntityName", other_id)
            if other_name == Terrain.name:
                self.bounced = True
                Report.info("{} has bounced off the terrain.".format(self.name))

        def enable_handler(self):
            self.handler = phys.CollisionNotificationBusHandler()
            self.handler.connect(self.id)
            self.handler.add_callback("OnCollisionBegin", self.on_collision_begin)
            self.handler.add_callback("OnCollisionEnd", self.on_collision_end)

    def is_close(actual, expected, buffer):
        return abs(actual - expected) < buffer

    def balls_are_aligned(balls_list):
        aligned = True

        for ball in balls_list:
            # check x axis per level setup
            if ball.name == "Ball_Default":
                if not is_close(ball.world_location_start.x, X_POSITION_DEFAULT, POSITION_BUFFER):
                    Report.info("Ball_Default is not close enough to expected X position")
                    aligned = False

            elif ball.name == "Ball_Rubber":
                if not is_close(ball.world_location_start.x, X_POSITION_RUBBER, POSITION_BUFFER):
                    Report.info("Ball_Rubber is not close enough to expected X position")
                    aligned = False

            elif ball.name == "Ball_Concrete":
                if not is_close(ball.world_location_start.x, X_POSITION_CONCRETE, POSITION_BUFFER):
                    Report.info("Ball_Concrete is not close enough to expected X position")
                    aligned = False

            # check y axis per level setup
            if not is_close(ball.world_location_start.y, Y_POSITION_VALUE, POSITION_BUFFER):
                aligned = False
                Report.info("One or more balls are not close enough to expected Y position")

        return aligned

    def ball_heights_match(balls_list):
        heights_match = True

        for ball in balls_list:
            # check ball heights match each other (z axis)
            if ball.world_location_start.z != balls[0].world_location_start.z:
                heights_match = False
                Report.info("The balls are not falling from the same height.")
                Report.failure(Tests.same_starting_height)

        return heights_match

    helper.init_idle()

    # 1) Open level
    helper.open_level("Physics", "Material_CanBeAssignedToTerrain")

    # 2) Enter game mode
    helper.enter_game_mode(Tests.game_mode_enter)
    general.idle_wait_frames(NUM_WAIT_FRAMES_ENTITY_LOAD)

    # 3) Find entities
    Terrain.id = general.find_game_entity("Terrain")
    Terrain.name = azlmbr.entity.GameEntityContextRequestBus(azlmbr.bus.Broadcast, "GetEntityName", Terrain.id)

    ball_default = Sphere("Ball_Default")
    ball_rubber = Sphere("Ball_Rubber")
    ball_concrete = Sphere("Ball_Concrete")
    balls = (ball_rubber, ball_default, ball_concrete)

    Report.critical_result(Tests.find_terrain, Terrain.id.IsValid())
    Report.critical_result(Tests.find_ball_default, ball_default.id.IsValid())
    Report.critical_result(Tests.find_ball_rubber, ball_rubber.id.IsValid())
    Report.critical_result(Tests.find_ball_concrete, ball_concrete.id.IsValid())

    # 4) Check that gravity is disabled for all the balls initially
    gravity_disabled_for_all = True
    for ball in balls:
        if ball.gravity_enabled is True:
            gravity_disabled_for_all = False

    Report.result(Tests.all_gravity_disabled, gravity_disabled_for_all)

    # 5) Check that the balls are aligned and all falling from the same height
    balls_are_aligned = balls_are_aligned(balls)
    Report.critical_result(Tests.balls_are_aligned, balls_are_aligned)

    same_starting_height = ball_heights_match(balls)
    Report.critical_result(Tests.same_starting_height, same_starting_height)

    # Steps 6-9 run for each ball
    for ball in balls:
        # 6) Assign the tests and enable handlers to their respective spheres
        ball.assign_tests()
        ball.enable_handler()

        # 7) Enable gravity on ball entities
        ball.enable_gravity()

        # 8) Check that the balls collide with the PhysX Terrain
        helper.wait_for_condition(lambda: ball.bounced, TIMEOUT_IN_SECONDS)
        Report.result(ball.test_terrain_collide, ball.hit_ground)

        # 9) Wait for the ball to reach its peak height; record height and freeze it
        helper.wait_for_condition(ball.peak_height_reached, TIMEOUT_IN_SECONDS)
        Report.result(ball.test_peak_reached, ball.peak_reached)

    # 10) Compare the bounce heights of the balls
    # The restitution of rubber is greater than the default; the restitution of concrete is less than the default
    height_order_correct = ball_rubber.peak_height > ball_default.peak_height > ball_concrete.peak_height
    Report.result(Tests.bounce_height_order_correct, height_order_correct)

    # 11) Exit game mode and close the editor
    helper.exit_game_mode(Tests.game_mode_exit)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Material_CanBeAssignedToTerrain)
