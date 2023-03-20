"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

"""
Test case ID : C6090550
Test Case Title : Check that force region exerts world space force on rigid bodies (negative test)

"""


# fmt: off
class Tests:
    enter_game_mode           = ("Entered game mode",                                   "Failed to enter game mode")
    ball_exists               = ("The Ball entity was found",                           "The Ball entity was not found")
    forceregion_exists        = ("The ForceRegion entity was found",                    "The ForceRegion entity was not found")
    terrain_exists            = ("The Terrain entity was found",                        "The Terrain entity was not found")
    ball_over_force_region    = ("The ball is over the force region",                   "The ball is not over the force region")
    gravity_enabled_on_ball   = ("The ball has gravity enabled",                        "The ball gravity failed to enable")
    ball_entered_force_region = ("The ball entered the force region",                   "The ball did not enter the force region before timeout")
    ball_fell_through_region  = ("The ball fell through the force region",              "The ball did not exit force region below enter location")
    enter_exit_distance_close = ("The ball has passed through the entire force region", "Ball did not move the height of the force region")
    ball_hit_ground           = ("The ball collided with the PhysX Terrain",            "The ball did not collide with the PhysX Terrain before timeout")
    exit_game_mode            = ("Exited game mode",                                    "Couldn't exit game mode")
# fmt: on


def ForceRegion_ZeroWorldSpaceForceDoesNothing():
    """
    Summary:
        A ball is suspended above a cube shaped force region and a PhysX Terrain. The force is a world space force
        pointed in the positive Z direction with a magnitude of 10.

    Level Description:
        Ball - (entity): Mesh: Sphere shaped
                         PhysX Rigid Body: Gravity is enabled; Default settings
                         PhysX Collider: Sphere shape; offset (0.0, 0.0, 0.5); draw collider checked

        ForceRegion - (entity): Box Shape: Game view checked; Shape dimensions (3.0, 3.0, 3.0)
                                PhysX Force Region: World Space force; direction (0.0, 0.0, 1.0); magnitude 10.0
                                PhysX Collider: Box shape; Box dimensions (3.0, 3.0, 3.0); Trigger checked;
                                                Draw collider checked

        Terrain - (entity): PhysX Terrain: Default settings

    Expected Behavior:
        When game mode is entered, the ball entity will fall due to gravity and enter the force region.
        The region has a very weak force and therefore the ball will fall through the force region,
        where it will then collide with the PhysX Terrain.

    Test Steps:
        1) Open level
        2) Enter game mode
        3) Find/setup entities and handlers
        4) Check that the ball is over the force region
        5) Wait for ball to hit the ground
        6) Report if the ball entered the force region, fell through it, and collided with terrain in the alloted time
        7) Exit game mode and close the editor

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

    helper.init_idle()

    TIMEOUT_SECONDS = 3.0
    X_Y_Z_TOLERANCE = 1.5
    REGION_HEIGHT = 3.0
    TERRAIN_HEIGHT = 32.0

    def is_close(value1, value2, tolerance):
        """
        () -> bool
        Absolute value of difference of values being less than or equal to the tolerance
        """
        return abs(value1 - value2) <= tolerance

    def is_at_least(value1, value2, minimum):
        """
        () -> bool
        Subtraction of value2 from value1 being greater than or equal to the minimum
        """
        return (value1 - value2) >= minimum

    class Entity(object):
        def __init__(self, name):
            self.name = name
            self.id = general.find_game_entity(self.name)
            Report.critical_result(Tests.__dict__[self.name.lower() + "_exists"], self.id.IsValid())

        def get_location(self):
            # () -> Vector3
            return azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", self.id)

    class Ball(Entity):
        def __init__(self):
            super(Ball, self).__init__("Ball")
            self.location = self.get_location()
            self.enter_region_location = None
            self.exit_region_location = None
            self.enable_gravity()
            self.check_gravity_is_on()

        def enable_gravity(self):
            azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "SetGravityEnabled", self.id)

        def check_gravity_is_on(self):
            gravity_status = azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "IsGravityEnabled", self.id)
            Report.critical_result(Tests.__dict__["gravity_enabled_on_" + self.name.lower()], gravity_status)

        def check_alignment(self, region):
            # () -> bool
            Report.info_vector3(self.location, "Location of Ball: ")
            Report.info_vector3(region.location, "Location of ForceRegion: ")

            aligned = True
            if not is_close(self.location.x, region.location.x, X_Y_Z_TOLERANCE):
                aligned = False
                Report.info("The ball is not aligned in the X axis.")
            if not is_close(self.location.y, region.location.y, X_Y_Z_TOLERANCE):
                aligned = False
                Report.info("The ball is not aligned in the Y axis.")
            if (self.location.z - region.location.z) < X_Y_Z_TOLERANCE:
                aligned = False
                Report.info("The ball is not high enough to enter the force region.")
            if self.location.z < TERRAIN_HEIGHT:
                aligned = False
                Report.info("The ball is below the terrain.")
            return aligned

        def did_ball_fall_in_region_as_expected(self):
            # () -> bool
            if not self.exit_region_location:
                Report.info("The ball did not exit the force region in time.")
                return False
            return (self.enter_region_location.z - self.exit_region_location.z) > X_Y_Z_TOLERANCE

        def check_enter_vs_exit_locations(self):
            """
            Check that the ball enter location is above the exit location (ball successfully fell through the force
            region) and check that the distance between the enter/exit locations is at least the height of the region

            NOTE: The ball will "enter" the force region when the bottom edge of its collision box enters the region and
                  will "exit" when it is completely outside the force region (top edge leaves region)
            """
            Report.info_vector3(self.enter_region_location, "Ball entering location: ")
            Report.info_vector3(self.exit_region_location, "Ball exiting location: ")

            Report.result(Tests.ball_fell_through_region, self.did_ball_fall_in_region_as_expected())
            Report.result(
                Tests.enter_exit_distance_close,
                is_at_least(self.enter_region_location.z, self.exit_region_location.z, REGION_HEIGHT),
            )

    class ForceRegion(Entity):
        def __init__(self, ball):
            super(ForceRegion, self).__init__("ForceRegion")
            self.ball = ball  # This creates a reference point to access the ball without looking in global scope
            self.location = self.get_location()
            self.ball_entered_trigger_area = False
            self.trigger_handler = azlmbr.physics.TriggerNotificationBusHandler()
            self.connect_trigger_handler()

        def on_trigger_enter(self, args):
            other_id = args[0]
            if other_id.Equal(self.ball.id):
                Report.info("Ball has entered the force region trigger area.")
                self.ball_entered_trigger_area = True
                self.ball.enter_region_location = self.ball.get_location()

        def on_trigger_exit(self, args):
            other_id = args[0]
            if other_id.Equal(self.ball.id):
                Report.info("Ball has exited the force region trigger area.")
                self.ball.exit_region_location = self.ball.get_location()

        def connect_trigger_handler(self):
            self.trigger_handler.connect(self.id)
            self.trigger_handler.add_callback("OnTriggerEnter", self.on_trigger_enter)
            self.trigger_handler.add_callback("OnTriggerExit", self.on_trigger_exit)

    class Terrain(Entity):
        def __init__(self, ball):
            super(Terrain, self).__init__("Terrain")
            self.ball = ball  # This creates a reference point to access the ball without looking in global scope
            self.ball_collided = False
            self.collision_handler = azlmbr.physics.CollisionNotificationBusHandler()
            self.connect_collision_handler()

        def on_collision_begin(self, args):
            other_id = args[0]
            if other_id.Equal(self.ball.id):
                Report.info("Ball struck PhysX Terrain")
                self.ball_collided = True

        def connect_collision_handler(self):
            self.collision_handler.connect(self.id)
            self.collision_handler.add_callback("OnCollisionBegin", self.on_collision_begin)

    # 1) Open level
    helper.open_level("Physics", "ForceRegion_ZeroWorldSpaceForceDoesNothing")

    # 2) Enter game mode
    helper.enter_game_mode(Tests.enter_game_mode)

    # 3) Find/setup entities and handlers
    ball = Ball()
    region = ForceRegion(ball)
    terrain = Terrain(ball)

    # 4) Check that the ball is over the force region
    Report.critical_result(Tests.ball_over_force_region, ball.check_alignment(region))

    # 5) Wait for ball to hit the ground
    helper.wait_for_condition(lambda: terrain.ball_collided, TIMEOUT_SECONDS)

    # 6) Report if the ball entered the force region, fell through it, and collided with terrain in the alloted time
    Report.result(Tests.ball_entered_force_region, region.ball_entered_trigger_area)
    ball.check_enter_vs_exit_locations()
    Report.result(Tests.ball_hit_ground, terrain.ball_collided)

    # 7) Exit game mode and close the editor
    helper.exit_game_mode(Tests.exit_game_mode)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(ForceRegion_ZeroWorldSpaceForceDoesNothing)
