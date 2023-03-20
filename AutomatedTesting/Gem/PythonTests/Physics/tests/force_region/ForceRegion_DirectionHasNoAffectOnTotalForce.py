"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# Test case ID : C12868578
# Test Case Title : Check that World space and local space force direction doesn't affect magnitude of force exerted


# fmt: off
class Tests:
    enter_game_mode      = ("Entered game mode",                        "Failed to enter game mode")
    exit_game_mode       = ("Exited game mode",                         "Couldn't exit game mode")
    entity_position      = ("All entities in good relative position",   "Not all entities in correct position")
    sphere_collisions    = ("All spheres collided with Force Regions",  "Not All spheres collided")
    initial_velocity     = ("Spheres started moving correctly",         "Spheres not moving correctly")
    velocity_updated     = ("Sphere velocities updated",                "Sphere velocities didn't update")

    # Z Direction
    sphere_0_found       = ("sphere_0 is found",                        "sphere_0 is not found")
    sphere_1_found       = ("sphere_1 is found",                        "sphere_1 is not found")
    force_region_0_found = ("force_region_0 is found",                  "force_region_0 is not found")
    force_region_1_found = ("force_region_1 is found",                  "force_region_1 is not found")
    local_force_mag_z    = ("z-axis Local Space force magnitude valid", "z-axis Local Space force magnitude invalid")
    local_force_dir_z    = ("z-axis Local Space force direction valid", "z-axis Local Space force direction invalid")
    world_force_mag_z    = ("z-axis World Space force magnitude valid", "z-axis World Space force magnitude invalid")
    world_force_dir_z    = ("z-axis World Space force direction valid", "z-axis World Space force direction invalid")

    # X Direction
    sphere_2_found       = ("sphere_2 is found",                        "sphere_2 is not found")
    sphere_3_found       = ("sphere_3 is found",                        "sphere_3 is not found")
    force_region_2_found = ("force_region_2 is found",                  "force_region_2 is not found")
    force_region_3_found = ("force_region_3 is found",                  "force_region_3 is not found")
    local_force_mag_x    = ("x-axis Local Space force magnitude valid", "x-axis Local Space force magnitude invalid")
    local_force_dir_x    = ("x-axis Local Space force direction valid", "x-axis Local Space force direction invalid")
    world_force_mag_x    = ("x-axis World Space force magnitude valid", "x-axis World Space force magnitude invalid")
    world_force_dir_x    = ("x-axis World Space force direction valid", "x-axis World Space force direction invalid")

    # Y Direction
    sphere_4_found       = ("sphere_4 is found",                        "sphere_4 is not found")
    sphere_5_found       = ("sphere_5 is found",                        "sphere_5 is not found")
    force_region_4_found = ("force_region_4 is found",                  "force_region_4 is not found")
    force_region_5_found = ("force_region_5 is found",                  "force_region_5 is not found")
    local_force_mag_y    = ("y-axis Local Space force magnitude valid", "y-axis Local Space force magnitude invalid")
    local_force_dir_y    = ("y-axis Local Space force direction valid", "y-axis Local Space force direction invalid")
    world_force_mag_y    = ("y-axis World Space force magnitude valid", "y-axis World Space force magnitude invalid")
    world_force_dir_y    = ("y-axis World Space force direction valid", "y-axis World Space force direction invalid")
# fmt: on


def ForceRegion_DirectionHasNoAffectOnTotalForce():
    """
    Summary: Check that world and local space force direction should not affect magnitude of force exerted on entity.

    Level Description:
    sphere_0 - Directly above force_region_0 with velocity of 10.0 in the negative z direction; has sphere shape
        collider, rigid body, and sphere shape
    sphere_1 - Directly above force_region_1 with velocity of 10.0 in the negative z direction; has sphere shape
        collider, rigid body, and sphere shape
    force_region_0 - Directly below sphere_0 with world space force of magnitude 100.0 and direction vector of
        <0.0,0.0,999.0>; has box shape collider and force region
    force_region_1 - Directly below sphere_1 with local space force of magnitude 100.0 and direction vector of
        <0.0,0.0,999.0>; has box shape collider and force region

    Expected Behavior: Both spheres bounce off of there respective force regions with a force of magnitude that is close
        to 100.0 in positive z direction. The direction is normalized from the manual entered direction input.

    Test Steps:
    1) Open Level
    2) Enter Game Mode
    3) Set up and validate entities
    4) Wait for collision
    5) Wait for velocities to become positive
    6) Log and validate results
    7) Exit Game Mode
    8) Close Editor

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
    FLOAT_THRESHOLD = sys.float_info.epsilon
    TIMEOUT = 1
    MAGNITUDE_THRESHOLD = 0.1
    FORCE_VECTOR_THRESHOLD = 0.001

    # Helper Functions
    class Entity:
        def __init__(self, name):
            # type (str, hex) -> None
            self.id = general.find_game_entity(name)
            self.name = name
            self.collision_happened = False
            # ID validation
            self.found = Tests.__dict__[self.name + "_found"]
            Report.critical_result(self.found, self.id.isValid())

        @property
        def position(self):
            # type () -> Vector3
            return azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", self.id)

    class Sphere(Entity):
        def __init__(self, name, axis, force_region):
            Entity.__init__(self, name)
            self.paired_force_region = force_region
            self.axis = axis
            self.force_vector = None
            self.force_magnitude = None
            # Set Handler
            self.handler = azlmbr.physics.ForceRegionNotificationBusHandler()
            self.handler.connect(None)
            self.handler.add_callback("OnCalculateNetForce", self.on_calculate_net_force)
            # Report initial values
            Report.info_vector3(self.position, "{} initial position: ".format(self.name))
            Report.info_vector3(self.velocity, "{} initial velocity: ".format(self.name))

        @property
        def velocity(self):
            # type () -> Vector3
            return azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "GetLinearVelocity", self.id)

        @property
        def is_moving_in_positive_direction(self):
            # type () -> bool
            # A List of the attribute names for the velocity (Vector3)
            axis = ["x", "y", "z"]
            # Finds the index in the list of the attribute in which the sphere is moving in
            index = axis.index(self.axis)
            # Checking that we are moving along that axis
            moving_component = getattr(self.velocity, axis[index]) > 0.0
            # Getting rid of moving axis from list
            axis.pop(index)
            # Checking that the sphere is not moving along either of the remaining two axis.
            stationary_components = (
                abs(getattr(self.velocity, axis[0])) < FLOAT_THRESHOLD
                and abs(getattr(self.velocity, axis[1])) < FLOAT_THRESHOLD
            )
            return moving_component and stationary_components

        def report_values(self):
            # type () -> None
            # Reports final position and velocity information
            Report.info_vector3(self.position, "{} final position: ".format(self.name))
            Report.info_vector3(self.velocity, "{} final velocity: ".format(self.name))

        def on_calculate_net_force(self, args):
            # type (list) -> None
            # Flips the collision happened boolean for the sphere object and prints the force values.
            if self.paired_force_region.id.Equal(args[0]) and self.id.equal(args[1]) and not self.collision_happened:
                self.collision_happened = True
                self.force_vector = args[2]
                self.force_magnitude = args[3]
                # Report force vector information
                Report.info_vector3(self.force_vector, "{} had following force vector applied".format(self.name))
                Report.info("{} is the applied force magnitude".format(self.force_magnitude))

    def validate_local_force_results(sphere):
        # type (Sphere) -> None
        local_force_direction = Tests.__dict__["local_force_dir_{}".format(sphere.axis)]
        local_force_magnitude = Tests.__dict__["local_force_mag_{}".format(sphere.axis)]

        Report.result(local_force_direction, check_applied_force_vector(sphere.force_vector))
        force_region_magnitude = azlmbr.physics.ForceLocalSpaceRequestBus(azlmbr.bus.Event,"GetMagnitude", sphere.paired_force_region.id)
        print(force_region_magnitude)
        print("LOOKKKK ABOVE!")
        Report.result(local_force_magnitude, abs(sphere.force_magnitude - force_region_magnitude) < MAGNITUDE_THRESHOLD)

    def validate_world_force_results(sphere):
        # type (Sphere) -> None
        world_force_direction = Tests.__dict__["world_force_dir_{}".format(sphere.axis)]
        world_force_magnitude = Tests.__dict__["world_force_mag_{}".format(sphere.axis)]

        Report.result(world_force_direction, check_applied_force_vector(sphere.force_vector))
        force_region_magnitude = azlmbr.physics.ForceWorldSpaceRequestBus(azlmbr.bus.Event,"GetMagnitude", sphere.paired_force_region.id)
        Report.result(world_force_magnitude, abs(sphere.force_magnitude - force_region_magnitude) < MAGNITUDE_THRESHOLD)

    def check_pair_position(sphere):
        # type (Sphere) -> bool
        # Ensures sphere lines up with its associated force region
        force_region_position = sphere.paired_force_region.position
        axis = ["x", "y", "z"]
        index = axis.index(sphere.axis)
        offset_component = getattr(force_region_position, axis[index]) < getattr(sphere.position, axis[index])
        axis.pop(index)
        zero_components = (
            abs(getattr(force_region_position, axis[0]) - getattr(sphere.position, axis[0])) < FLOAT_THRESHOLD
            and abs(getattr(force_region_position, axis[1]) - getattr(sphere.position, axis[1])) < FLOAT_THRESHOLD
        )
        return offset_component and zero_components

    def check_applied_force_vector(vector):
        # type (Sphere) -> bool
        # Ensures the force vector is within expected threshold. The components of the vector can either be 0 or 1
        axis = ["x", "y", "z"]
        return all(
            [
                True
                for component in axis
                if abs(getattr(vector, component) - 1.00) < FORCE_VECTOR_THRESHOLD
                or abs(getattr(vector, component)) < FORCE_VECTOR_THRESHOLD
            ]
        )

    # Main Script
    helper.init_idle()
    # 1) Open Level
    helper.open_level("Physics", "ForceRegion_DirectionHasNoAffectOnTotalForce")

    # 2) Enter Game Mode
    helper.enter_game_mode(Tests.enter_game_mode)

    # 3) Set up and validate entities
    force_region_0 = Entity("force_region_0")
    force_region_1 = Entity("force_region_1")
    force_region_2 = Entity("force_region_2")
    force_region_3 = Entity("force_region_3")
    force_region_4 = Entity("force_region_4")
    force_region_5 = Entity("force_region_5")

    sphere_0 = Sphere("sphere_0", "z", force_region_0)
    sphere_1 = Sphere("sphere_1", "z", force_region_1)
    sphere_2 = Sphere("sphere_2", "x", force_region_2)
    sphere_3 = Sphere("sphere_3", "x", force_region_3)
    sphere_4 = Sphere("sphere_4", "y", force_region_4)
    sphere_5 = Sphere("sphere_5", "y", force_region_5)
    sphere_list = [sphere_0, sphere_1, sphere_2, sphere_3, sphere_4, sphere_5]
    local_force_list = [sphere_1, sphere_3, sphere_5]
    world_force_list = [sphere_0, sphere_2, sphere_4]

    Report.critical_result(
        Tests.entity_position, all([check_pair_position(sphere) for sphere in sphere_list])
    )

    Report.critical_result(
        Tests.initial_velocity,
        all([not sphere.is_moving_in_positive_direction for sphere in sphere_list]),
    )

    # 4) Wait for collision
    Report.critical_result(
        Tests.sphere_collisions,
        helper.wait_for_condition(
            lambda: all([sphere.collision_happened for sphere in sphere_list]), TIMEOUT
        ),
    )

    # 5) Wait for velocities to become positive
    Report.critical_result(
        Tests.velocity_updated,
        helper.wait_for_condition(
            lambda: all([sphere.is_moving_in_positive_direction for sphere in sphere_list]), TIMEOUT
        ),
    )

    # 6) Log and validate results
    [validate_local_force_results(sphere) for sphere in local_force_list]
    [validate_world_force_results(sphere) for sphere in world_force_list]

    [sphere.report_values() for sphere in sphere_list]

    # 7) Exit Game Mode
    helper.exit_game_mode(Tests.exit_game_mode)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(ForceRegion_DirectionHasNoAffectOnTotalForce)
