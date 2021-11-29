"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# Test case ID : C5959809
# Test Case Title : Verify Force Region Rotational Offset



# fmt:off
class Tests:
    # General tests
    enter_game_mode = ("Entered game mode",               "Failed to enter game mode")
    exit_game_mode  = ("Exited game mode",                "Couldn't exit game mode")
    test_completed  = ("The test successfully completed", "The test timed out")
    # ***** Entities found *****
    # Force Regions
    force_region_x_found          = ("Force Region for X axis test was found",          "Force Region for X axis test was NOT found")
    force_region_y_found          = ("Force Region for Y axis test was found",          "Force Region for Y axis test was NOT found")
    force_region_z_found          = ("Force Region for Z axis test was found",          "Force Region for Z axis test was NOT found")
    # Force Region Pass Boxes
    force_region_pass_box_x_found = ("Force Region Pass Box for X axis test was found", "Force Region Pass Box for X axis test was NOT found")
    force_region_pass_box_y_found = ("Force Region Pass Box for Y axis test was found", "Force Region Pass Box for Y axis test was NOT found")
    force_region_pass_box_z_found = ("Force Region Pass Box for Z axis test was found", "Force Region Pass Box for Z axis test was NOT found")
    # External Pass Boxes
    external_pass_box_x_found     = ("External Pass Box for X axis test was found",     "External Pass Box for X axis test was NOT found")
    external_pass_box_y_found     = ("External Pass Box for Y axis test was found",     "External Pass Box for Y axis test was NOT found")
    external_pass_box_z_found     = ("External Pass Box for Z axis test was found",     "External Pass Box for Z axis test was NOT found")
    # Force Region Fail Boxes
    force_region_fail_box_x_found = ("Force Region Fail Box for X axis test was found", "Force Region Fail Box for X axis test was NOT found")
    force_region_fail_box_y_found = ("Force Region Fail Box for Y axis test was found", "Force Region Fail Box for Y axis test was NOT found")
    force_region_fail_box_z_found = ("Force Region Fail Box for Z axis test was found", "Force Region Fail Box for Z axis test was NOT found")
    # External Fail Boxes
    external_fail_box_x_found     = ("External Fail Box for X axis test was found",     "External Fail Box for X axis test was NOT found")
    external_fail_box_y_found     = ("External Fail Box for Y axis test was found",     "External Fail Box for Y axis test was NOT found")
    external_fail_box_z_found     = ("External Fail Box for Z axis test was found",     "External Fail Box for Z axis test was NOT found")
    # Pass spheres
    sphere_pass_x_found           = ("Pass Sphere for X axis test was found",           "Pass Sphere for X axis test was NOT found")
    sphere_pass_y_found           = ("Pass Sphere for Y axis test was found",           "Pass Sphere for Y axis test was NOT found")
    sphere_pass_z_found           = ("Pass Sphere for Z axis test was found",           "Pass Sphere for Z axis test was NOT found")
    # Bounce Spheres
    sphere_bounce_x_found         = ("Bounce Sphere for X axis test was found",         "Bounce Sphere for X axis test was NOT found")
    sphere_bounce_y_found         = ("Bounce Sphere for Y axis test was found",         "Bounce Sphere for Y axis test was NOT found")
    sphere_bounce_z_found         = ("Bounce Sphere for Z axis test was found",         "Bounce Sphere for Z axis test was NOT found")

    # ****** Entities' results ******
    # Force Regions
    force_region_x_mag_result      = ("Force Region for X axis magnitude exerted was as expected",         "Force Region for X axis magnitude exerted was NOT as expected")
    force_region_y_mag_result      = ("Force Region for Y axis magnitude exerted was as expected",         "Force Region for Y axis magnitude exerted was NOT as expected")
    force_region_z_mag_result      = ("Force Region for Z axis magnitude exerted was as expected",         "Force Region for Z axis magnitude exerted was NOT as expected")
    force_region_x_norm_result     = ("Force Region for X axis normal exerted was as expected",            "Force Region for X axis normal exerted was NOT as expected")
    force_region_y_norm_result     = ("Force Region for Y axis normal exerted was as expected",            "Force Region for Y axis normal exerted was NOT as expected")
    force_region_z_norm_result     = ("Force Region for Z axis normal exerted was as expected",            "Force Region for Z axis normal exerted was NOT as expected")
    # Force Region Pass Boxes
    force_region_pass_box_x_result = ("Force Region Pass Box for X axis collided with exactly one sphere", "Force Region Pass Box for X axis DID NOT collide with exactly one sphere")
    force_region_pass_box_y_result = ("Force Region Pass Box for Y axis collided with exactly one sphere", "Force Region Pass Box for Y axis DID NOT collide with exactly one sphere")
    force_region_pass_box_z_result = ("Force Region Pass Box for Z axis collided with exactly one sphere", "Force Region Pass Box for Z axis DID NOT collide with exactly one sphere")
    # External Pass Boxes
    external_pass_box_x_result     = ("External Pass Box for X axis collided with exactly one sphere",     "External Pass Box for X axis DID NOT collide with exactly one sphere")
    external_pass_box_y_result     = ("External Pass Box for Y axis collided with exactly one sphere",     "External Pass Box for Y axis DID NOT collide with exactly one sphere")
    external_pass_box_z_result     = ("External Pass Box for Z axis collided with exactly one sphere",     "External Pass Box for Z axis DID NOT collide with exactly one sphere")
    # Force Region Fail Boxes
    force_region_fail_box_x_result = ("Force Region Fail Box for X axis collided with no spheres",         "Force Region Fail Box for X axis DID collide with a sphere")
    force_region_fail_box_y_result = ("Force Region Fail Box for Y axis collided with no spheres",         "Force Region Fail Box for Y axis DID collide with a sphere")
    force_region_fail_box_z_result = ("Force Region Fail Box for Z axis collided with no spheres",         "Force Region Fail Box for Z axis DID collide with a sphere")
    # External Fail Boxes
    external_fail_box_x_result     = ("External Fail Box for X axis collided with no spheres",             "External Fail Box for X axis DID collide with a sphere")
    external_fail_box_y_result     = ("External Fail Box for Y axis collided with no spheres",             "External Fail Box for Y axis DID collide with a sphere")
    external_fail_box_z_result     = ("External Fail Box for Z axis collided with no spheres",             "External Fail Box for Z axis DID collide with a sphere")
    # Pass spheres
    sphere_pass_x_result           = ("Pass Sphere for X axis collided with expected Box",                 "Pass Sphere for X axis DID NOT collide with expected Box")
    sphere_pass_y_result           = ("Pass Sphere for Y axis collided with expected Box",                 "Pass Sphere for Y axis DID NOT collide with expected Box")
    sphere_pass_z_result           = ("Pass Sphere for Z axis collided with expected Box",                 "Pass Sphere for Z axis DID NOT collide with expected Box")
    # Bounce Spheres
    sphere_bounce_x_result         = ("Bounce Sphere for X axis collided with expected Box",               "Bounce Sphere for X axis DID NOT collide with expected Box")
    sphere_bounce_y_result         = ("Bounce Sphere for Y axis collided with expected Box",               "Bounce Sphere for Y axis DID NOT collide with expected Box")
    sphere_bounce_z_result         = ("Bounce Sphere for Z axis collided with expected Box",               "Bounce Sphere for Z axis DID NOT collide with expected Box")
# fmt:on

    @staticmethod
    # Test tuple accessor via string
    def get_test(test_name):
        if test_name in Tests.__dict__:
            return Tests.__dict__[test_name]
        else:
            return None


def ForceRegion_RotationalOffset():
    """
    Summary:
    Force Region rotational offset is tested for each of the 3 axises (X, Y, and Z). Each axis's test has one
    ForceRegion, two spheres and four boxes. By monitoring which box each sphere collides with we can validate the
    integrity of the ForceRegions rotational offset.

    Level Description:
    Each axis's test has the following entities:
    one force region - set for point force and with it's collider rotationally offset (on the axis in test).
    two spheres - one positioned near the transform of the force region, one positioned near the [offset] collider for
        the force region
    four boxes - One box is positioned inside the force region's transform, one inside the force region's [offset]
        collider. The other two boxes are positioned behind the two spheres (relative to the direction they will be
        initially traveling)

    Expected Behavior:
    All three axises' tests run in parallel. when the tests begin, the spheres should move toward their expected
    force regions. The spheres positioned to collide with their region's [offset] collider should be forced backwards
    before entering the force region and collide with the box behind it. The spheres positioned by their force region's
    transforms should pass straight into the transform and collide with the box inside the transform.
    The boxes inside the Force Regions' [offset] colliders and the boxes behind the spheres set to move into the Force
    Regions' transforms should not register any collisions.

    Steps:
    1) Open level and enter game mode
    2) Set up tests and variables
    3) Wait for test results (or time out)
        (Report results)
    4) Exit game mode and close the editor

    :return: None
    """

    import os
    import sys
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    import azlmbr.legacy.general as general
    import azlmbr.math as azmath
    import azlmbr.bus
    import azlmbr

    # Constants
    CLOSE_ENOUGH = 0.01  # Close enough threshold for comparing floats
    TIME_OUT = 2.0  # Time out (in seconds) until test is aborted
    FORCE_MAGNITUDE = 1000.0  # Point force magnitude for Force Regions
    SPEED = 3.0  # Initial speed (in m/s) of the moving spheres.

    # Full list for all spheres. Used for EntityId look up in event handlers
    all_spheres = []

    # Entity base class handles very general entity initialization
    # Should be treated as a "virtual" class and all implementing child
    # classes should implement a "self.result()" function referenced in EntityBase::report(self)
    class EntityBase:
        def __init__(self, name):
            # type: (str) -> None
            self.name = name
            self.print_list = []
            self.id = general.find_game_entity(name)
            found_test = Tests.get_test(name + "_found")
            Report.critical_result(found_test, self.id.IsValid())

        # Reports this entity's result. Implicitly calls "get" on result.
        # Subclasses implement their own definition of a successful result
        def report(self):
            # type: () -> None
            result_test = Tests.get_test(self.name + "_result")
            Report.result(result_test, self.result())

        # Prints the print queue (with decorated header) if not empty
        def print_log(self):
            # type: () -> None
            if self.print_list:
                Report.info("*********** {} **********".format(self))
                for line in self.print_list:
                    Report.info(line)
                Report.info("")

        # Quick string cast, returns entity name
        def __str__(self):
            # type: () -> str
            return self.name

    # ForceRegion handles all the data and behavior associated with a ForceRegion (for this test)
    # They simply wait for a Sphere to collide with them. On collision they store the calculated force
    # magnitude for verification.
    class ForceRegion(EntityBase):
        def __init__(self, name, magnitude):
            # type: (str, float) -> None
            EntityBase.__init__(self, name)
            self.expected_magnitude = magnitude
            self.actual_magnitude = None
            self.expected_normal = None
            self.actual_normal = None
            # Set point force Magnitude
            azlmbr.physics.ForcePointRequestBus(azlmbr.bus.Event, "SetMagnitude", self.id, magnitude)
            # Set up handler
            self.handler = azlmbr.physics.ForceRegionNotificationBusHandler()
            self.handler.connect(None)
            self.handler.add_callback("OnCalculateNetForce", self.on_calc_force)

        # Callback function for OnCalculateNetForce event
        def on_calc_force(self, args):
            # type: ([EntityId, EntityId, azmath.Vector3, float]) -> None
            if self.id.Equal(args[0]) and self.actual_magnitude is None:
                for sphere in all_spheres:
                    if sphere.id.Equal(args[1]):
                        # Log event in print queue (for me and for the sphere)
                        self.print_list.append("Exerting force on {}:".format(sphere))
                        sphere.print_list.append("Force exerted by {}".format(self))
                        # Save calculated data to be compared later
                        self.actual_normal = args[2]
                        self.actual_magnitude = args[3]
                        pos = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", self.id)
                        sphere_pos = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", sphere.id)
                        self.expected_normal = sphere_pos.Subtract(pos).GetNormalizedSafe()
                        # Add expected/actual to print queue
                        self.print_list.append("Force Vector: ")
                        self.print_list.append(
                            "  Expected: ({:.2f}, {:.2f}, {:.2f})".format(
                                self.expected_normal.x, self.expected_normal.y, self.expected_normal.z
                            )
                        )
                        self.print_list.append(
                            "  Actual: ({:.2f}, {:.2f}, {:.2f})".format(
                                self.actual_normal.x, self.actual_normal.y, self.actual_normal.z
                            )
                        )
                        self.print_list.append("Force Magnitude: ")
                        self.print_list.append("  Expected: {}".format(self.expected_magnitude))
                        self.print_list.append("  Actual: {:.2f}".format(self.actual_magnitude))

        # EntityBase::report() overload.
        # Force regions have 2 test tuples to report on
        def report(self):
            magnitude_test = Tests.get_test(self.name + "_mag_result")
            normal_test = Tests.get_test(self.name + "_norm_result")
            Report.result(magnitude_test, self.magnitude_result())
            Report.result(normal_test, self.normal_result())

        # Test result calculations
        # Used in EntityBase for reporting results
        def result(self):
            # type: () -> bool
            return self.magnitude_result() and self.normal_result()

        def magnitude_result(self):
            # type: () -> bool
            return (
                    self.actual_magnitude is not None
                    and abs(self.actual_magnitude - self.expected_magnitude) < CLOSE_ENOUGH
            )

        def normal_result(self):
            # type: () -> bool
            return (
                    self.actual_normal is not None
                    and self.expected_normal is not None
                    and self.expected_normal.IsClose(self.actual_normal, CLOSE_ENOUGH)
            )

    # Spheres are the objects that test the force regions. They store an expected collision entity and an
    # actual collision entity
    class Sphere(EntityBase):
        def __init__(self, name, initial_velocity, expected_collision):
            # type: (str, azmath.Vector3, EntityBase) -> None
            EntityBase.__init__(self, name)
            self.initial_velocity = initial_velocity
            azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "SetLinearVelocity", self.id, initial_velocity)
            self.print_list.append(
                "Initial velocity: ({:.2f}, {:.2f}, {:.2f})".format(
                    initial_velocity.x, initial_velocity.y, initial_velocity.z
                )
            )
            self.expected_collision = expected_collision
            self.print_list.append("Expected Collision: {}".format(expected_collision))
            self.actual_collision = None
            self.active = True
            self.force_normal = None

        # Registers a collision with this sphere. Saves a reference to the colliding entity for processing later.
        # Deactivate self after collision is registered.
        def collide(self, collision_entity):
            # type: (EntityBase) -> None
            # Log the event
            self.print_list.append("Collided with {}".format(collision_entity))
            self.actual_collision = collision_entity
            # Deactivate self
            azlmbr.entity.GameEntityContextRequestBus(azlmbr.bus.Broadcast, "DeactivateGameEntity",
                                                      self.id)
            self.active = False

        # Calculates result
        # Used in EntityBase for reporting results
        def result(self):
            if self.actual_collision is None:
                return False
            else:
                return self.expected_collision.id.Equal(self.actual_collision.id)

    # Box entities wait for a collision with a sphere as a means of validation the force region's offset
    # worked according to plan.
    class Box(EntityBase):
        def __init__(self, name, expected_sphere_collisions):
            # type: (str, int) -> None
            EntityBase.__init__(self, name)
            self.spheres_collided = 0
            self.expected_sphere_collisions = expected_sphere_collisions
            # Set up handler
            self.handler = azlmbr.physics.CollisionNotificationBusHandler()
            self.handler.connect(self.id)
            self.handler.add_callback("OnCollisionBegin", self.on_collision_begin)

        # Callback function for OnCollisionBegin event
        def on_collision_begin(self, args):
            for sphere in all_spheres:
                if sphere.id.Equal(args[0]):
                    # Log event
                    self.print_list.append("Collided with {}".format(sphere))
                    # Register collision with sphere
                    sphere.collide(self)
                    self.spheres_collided += 1  # Count collisions for validation later
                    break

        # Calculates test result
        # Used in EntityBase for reporting results
        def result(self):
            return self.spheres_collided == self.expected_sphere_collisions

    # Manages the entities required to run the test for one axis (X, Y, or Z)
    class AxisTest:
        def __init__(self, axis, init_velocity):
            # type: (str, azmath.Vector3) -> None
            self.name = axis + " axis test"
            self.force_region = ForceRegion("force_region_" + axis, FORCE_MAGNITUDE)
            self.spheres = [
                Sphere("sphere_pass_" + axis, init_velocity, Box("force_region_pass_box_" + axis, 1)),
                Sphere("sphere_bounce_" + axis, init_velocity, Box("external_pass_box_" + axis, 1)),
            ]
            self.boxes = [
                 Box("external_fail_box_" + axis, 0),
                 Box("force_region_fail_box_" + axis, 0)
            ] + [
                 sphere.expected_collision for sphere in self.spheres
                 # Gets the Boxes passed to spheres on init
            ]
            # Full list for all entities this test is responsible for
            self.all_entities = self.boxes + self.spheres + [self.force_region]
            # Add spheres to global "lookup" list
            all_spheres.extend(self.spheres)

        # Checks for all entities' test passing conditions
        def passed(self):
            return all([e.result() for e in self.all_entities])

        # Returns true when this test has completed (i.e. when the spheres have collided and are deactivated)
        def completed(self):
            return all([not sphere.active for sphere in self.spheres])

        # Reports results for all entities in this test
        def report(self):
            Report.info("::::::::::::::::::::::::::::: {} Results :::::::::::::::::::::::::::::".format(self.name))
            for entity in self.all_entities:
                entity.report()

        # Prints the logs for all entities in this test
        def print_log(self):
            Report.info("::::::::::::::::::::::::::::: {} Log :::::::::::::::::::::::::::::".format(self.name))
            for entity in self.all_entities:
                entity.print_log()

    # *********** Execution Code ***********

    # 1) Open level
    helper.init_idle()
    helper.open_level("Physics", "ForceRegion_RotationalOffset")
    helper.enter_game_mode(Tests.enter_game_mode)

    # 2) Variable set up
    # Initial velocities for the three different directions spheres will be moving
    x_vel = azmath.Vector3(SPEED, 0.0, 0.0)
    y_vel = azmath.Vector3(0.0, SPEED, 0.0)
    z_vel = azmath.Vector3(0.0, 0.0, SPEED)

    # The three tests, one for each axis
    axis_tests = [AxisTest("x", x_vel), AxisTest("y", y_vel), AxisTest("z", z_vel)]

    # 3) Wait for test results or time out
    Report.result(
        Tests.test_completed, helper.wait_for_condition(
            lambda: all([test.completed() for test in axis_tests]), TIME_OUT
        )
    )

    # Report results
    for test in axis_tests:
        test.report()

    # Print entity print queues for each failed test
    for test in axis_tests:
        if not test.passed():
            test.print_log()

    # 4) Exit game mode and close editor
    helper.exit_game_mode(Tests.exit_game_mode)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(ForceRegion_RotationalOffset)
