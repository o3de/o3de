"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# Test case ID : C5959810
# Test Case Title : Check that multiple forces in single force region create correct net force



# fmt: off
class Tests:
    enter_game_mode                  = ("Entered game mode",                    "Failed to enter game mode")
    sphere_exists                    = ("Sphere has been found",                "Sphere has not been found")
    force_region_exists              = ("Force Region has been found",          "Force Region has not been found")
    sphere_position_found            = ("Sphere position found",                "Sphere position not found")
    force_region_position_found      = ("Force Region position found",          "Force Region position not found")
    orientation_before_collision     = ("Sphere is above Force Region",         "Sphere is not above Force Region")
    sphere_velocity_found            = ("Sphere velocity found",                "Sphere velocity not found")
    sphere_velocity_before_collision = ("Sphere has valid initial velocity",    "Sphere initial velocity not valid")
    collision                        = ("Collision has occurred",               "No collision has occurred")
    force_applied                    = ("Forces were combined correctly",       "Forces were not applied correctly")
    new_velocity_applied             = ("Sphere velocity has been updated",     "Sphere velocity was never updated")
    orientation_after_collision      = ("Sphere is above and to the left",      "Entity orientation is not valid")
    sphere_velocity_post_collision   = ("Sphere velocity valid post-collision", "Sphere velocity no longer valid")
    force_region_has_not_moved       = ("Force Region has not moved",           "Force Region has somehow moved")
    exit_game_mode                   = ("Exited game mode",                     "Couldn't exit game mode")
# fmt: on


def ForceRegion_MultipleForcesInSameComponentCombineForces():
    # type: () -> None
    """
    Summary: Runs an automated test to ensure that separate forces in a force region add correctly

    Level Description:
    Sphere - Placed directly above the force region with an initial velocity in the -z direction;
        has PhysX Rigid Body, sphere shaped PhysX Collider, Sphere Shape
    Force Region - Placed directly under the sphere, has a point, and world space force in the
        z and negative x direction respectively; has PhysX Force Region, box shaped PhysX Collider

    Expected Behavior: Sphere collides with force region and is sent in the negative x and positive z direction

    Test Steps:
    1) Load Level
    2) Enter Game Mode
    3) Find Entities
    4) Validate initial positions and velocity
    5) Set up handler
    6) Wait for Sphere collision with Force Region
    7) Validate and Log Results
    8) Exit Game Mode
    9) Close Editor

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

    # Constants
    TIMEOUT = 1
    FLOAT_THRESHOLD = sys.float_info.epsilon

    # Helper Functions
    class Collision:
        happened = False
        force_vector = None
        force_magnitude = None
        velocity_now_updated = False

    class Entity:
        def __init__(self, name):
            self.id = general.find_game_entity(name)
            self.name = name
            self.initial_velocity = azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "GetLinearVelocity", self.id)
            self.final_velocity = None
            self.initial_position = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", self.id)
            self.final_position = None

        def get_final_position_and_velocity(self):
            self.final_velocity = azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "GetLinearVelocity", self.id)
            self.final_position = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", self.id)

        def report_sphere_values(self):
            Report.info_vector3(self.initial_position, "{} initial position: ".format(self.name))
            Report.info_vector3(self.initial_velocity, "{} initial velocity: ".format(self.name))

            Report.info_vector3(self.final_position, "{} final position: ".format(self.name))
            Report.info_vector3(self.final_velocity, "{} final velocity: ".format(self.name))

        def report_force_region_values(self):
            Report.info_vector3(self.initial_position, "{} initial position: ".format(self.name))
            Report.info_vector3(self.final_position, "{} final position: ".format(self.name))

    def validate_positions(collision_happened, sphere_position, force_region_position):
        if collision_happened:
            result = sphere_position.x < force_region_position.x
        else:
            result = abs(sphere_position.x - force_region_position.x) < FLOAT_THRESHOLD

        return (
            result
            and sphere_position.z > force_region_position.z
            and abs(sphere_position.y - force_region_position.y) < FLOAT_THRESHOLD
        )

    def validate_sphere_velocity(collision_happened, sphere_velocity_vector):
        if collision_happened:
            x_result = sphere_velocity_vector.x < 0
            z_result = sphere_velocity_vector.z > 0
        else:
            x_result = abs(sphere_velocity_vector.x) < FLOAT_THRESHOLD
            z_result = sphere_velocity_vector.z < 0

        return x_result and z_result and abs(sphere_velocity_vector.y) < FLOAT_THRESHOLD

    def vector_valid(vector, can_be_zero):
        if can_be_zero:
            return vector != None
        else:
            return vector != None and not vector.IsZero()

    def on_collision_begin(args):
        assert force_region.id.Equal(args[0])

        if sphere.id.equal(args[1]):
            Collision.happened = True
            Report.info("Collision has begun")
            if vector_valid(args[2], False):
                Collision.force_vector = args[2]
                Collision.force_magnitude = args[3]

    def force_valid(vector, magnitude):
        return magnitude > 0 and vector.x < 0 and vector.z > 0 and abs(vector.y) < FLOAT_THRESHOLD

    def velocity_update_check():
        current_velocity_vector = azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "GetLinearVelocity", sphere.id)
        velocity_updated = (
            current_velocity_vector.x < sphere.initial_velocity.x
            and current_velocity_vector.z > sphere.initial_velocity.z
        )
        if velocity_updated:
            Collision.velocity_now_updated = True

        return velocity_updated

    # Main Script
    helper.init_idle()

    # 1) Load Level
    helper.open_level("physics", "ForceRegion_MultipleForcesInSameComponentCombineForces")

    # 2) Enter Game Mode
    helper.enter_game_mode(Tests.enter_game_mode)

    # 3) Find Entities
    sphere = Entity("Sphere")
    force_region = Entity("Force_Region")

    Report.critical_result(Tests.sphere_exists, sphere.id.isValid())
    Report.critical_result(Tests.force_region_exists, force_region.id.isValid())

    # 4) Validate initial positions and velocity
    # Position validation
    Report.critical_result(Tests.sphere_position_found, vector_valid(sphere.initial_position, False))
    Report.critical_result(Tests.force_region_position_found, vector_valid(force_region.initial_position, False))
    # Velocity validation
    Report.critical_result(Tests.sphere_velocity_found, vector_valid(sphere.initial_velocity, False))

    # Value validation
    Report.critical_result(
        Tests.orientation_before_collision,
        validate_positions(Collision.happened, sphere.initial_position, force_region.initial_position),
    )
    Report.critical_result(
        Tests.sphere_velocity_before_collision, validate_sphere_velocity(Collision.happened, sphere.initial_velocity)
    )

    # 5) Set up handler
    handler = azlmbr.physics.ForceRegionNotificationBusHandler()
    handler.connect(None)
    handler.add_callback("OnCalculateNetForce", on_collision_begin)

    # 6) Wait for Sphere collision with Force Region and Velocity application
    helper.wait_for_condition(lambda: Collision.happened, TIMEOUT)
    helper.wait_for_condition(velocity_update_check, TIMEOUT)

    # 7) Validate and Log Results
    sphere.get_final_position_and_velocity()
    force_region.get_final_position_and_velocity()

    # Value validation
    Report.result(Tests.new_velocity_applied, Collision.velocity_now_updated)
    Report.result(Tests.collision, Collision.happened)
    Report.result(Tests.force_applied, force_valid(Collision.force_vector, Collision.force_magnitude))
    Report.result(
        Tests.orientation_after_collision,
        validate_positions(Collision.happened, sphere.final_position, force_region.final_position),
    )
    Report.result(
        Tests.sphere_velocity_post_collision, validate_sphere_velocity(Collision.happened, sphere.final_velocity)
    )
    Report.result(
        Tests.force_region_has_not_moved,
        force_region.final_position.Subtract(force_region.initial_position).IsZero(FLOAT_THRESHOLD),
    )
    # Value logging
    sphere.report_sphere_values()
    force_region.report_force_region_values()
    Report.info_vector3(Collision.force_vector, "Applied Force: ", Collision.force_magnitude)

    # 8) Exit Game Mode
    helper.exit_game_mode(Tests.exit_game_mode)



if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(ForceRegion_MultipleForcesInSameComponentCombineForces)
