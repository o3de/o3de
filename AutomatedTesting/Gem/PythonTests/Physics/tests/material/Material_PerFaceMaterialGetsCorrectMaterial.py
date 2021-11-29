"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# Test case ID : C4044697
# Test Case Title : Verify that each surface picks up the material assigned to it and behaves accordingly.


# fmt: off
class Tests:
    enter_game_mode                = ("Entered game mode",                  "Failed to enter game mode")
    exit_game_mode                 = ("Exited game mode",                   "Couldn't exit game mode")
    initial_orientaition_valid     = ("Initial entity orientation valid",   "Initial entity orientation not valid")
    final_orientation_valid        = ("Final entity orientation valid",     "Final entity orientation not valid")
    speed_comparision              = ("Sphere 1 is faster than Sphere 2",   "Sphere 1 is not faster than Sphere 2")

    # Sphere 0
    Sphere_0_found                 = ("Sphere 0 is valid",                  "Sphere 0 is not valid")
    Sphere_0_position_found        = ("Sphere 0 position is found",         "Sphere 0 position is not found")
    Sphere_0_velocity_found        = ("Sphere 0 velocity is found",         "Sphere 0 velocity is not found")
    Sphere_0_velocity_valid        = ("Sphere 0 velocity is valid",         "Sphere 0 velocity is not valid")
    Sphere_0_collided_with_perface = ("Sphere 0 collided w/Perface Entity", "Sphere 0 has not collided")
    Sphere_0_final_velocity_valid  = ("Sphere 0 final velocity is valid",   "Sphere 0 final velocity is not valid")

    # Sphere 1
    Sphere_1_found                 = ("Sphere 1 is valid",                  "Sphere 1 is not valid")
    Sphere_1_position_found        = ("Sphere 1 position is found",         "Sphere 1 position is not found")
    Sphere_1_velocity_found        = ("Sphere 1 velocity is found",         "Sphere 1 velocity is not found")
    Sphere_1_velocity_valid        = ("Sphere 1 velocity is valid",         "Sphere 1 velocity is not valid")
    Sphere_1_collided_with_perface = ("Sphere 1 collided w/Perface Entity", "Sphere 1 has not collided")
    Sphere_1_final_velocity_valid  = ("Sphere 1 final velocity is valid",   "Sphere 1 final velocity is not valid")

    # Sphere 2
    Sphere_2_found                 = ("Sphere 2 is valid",                  "Sphere 2 is not valid")
    Sphere_2_position_found        = ("Sphere 2 position is found",         "Sphere 2 position is not found")
    Sphere_2_velocity_found        = ("Sphere 2 velocity is found",         "Sphere 2 velocity is not found")
    Sphere_2_velocity_valid        = ("Sphere 2 velocity is valid",         "Sphere 2 velocity is not valid")
    Sphere_2_collided_with_perface = ("Sphere 2 collided w/Perface Entity", "Sphere 2 has not collided")
    Sphere_2_final_velocity_valid  = ("Sphere 2 final velocity is valid",   "Sphere 2 final velocity is not valid")

    # Perface Entity
    Perface_Entity_found           = ("Perface entity is valid",            "Perface entity is not valid")
    Perface_Entity_position_found  = ("Perface entity position found",      "Perface entity position not found")

# fmt: on


def Material_PerFaceMaterialGetsCorrectMaterial():
    """
    Summary: The perface has three different faces that can pick up different materials. To check that each face
        picks up the material assigned to it I send three spheres of the same material at each face to see the
        different reactions. If each sphere bounces away with the correct relative velocity it can be assumed
        that the Perface entity is picking up the materials properly.

    Level Description:
    Perface Entity - The perface entity is an entity with a custom mesh that allows for multiple materials to be
        applied to different parts of the mesh. In this case there seems to be three different areas of the mesh
        that can be assigned with different materials and interacted. One of three spheres is lined up to interact
        with one of each of the three areas. The mesh is included in the level "test.fbx". The entity is stationary
        with three spheres inline along the x and y axis: has a PhysX collider and a Mesh component.
    Sphere 0 - This entity is inline with the perface entity on the y axis and heading torward it with a velocity in
        the -y direction: has a sphere shaped PhysX Collider, a PhysX Rigid Body, and a Sphere Shape.
    Sphere 1 - This entity is inline with the perface entity on the x axis and heading torward it with a velocity in
        the -x direction: has a sphere shaped PhysX Collider, a PhysX Rigid Body, and a Sphere Shape.
    Sphere 2 - This entity is inline with the perface entity on the x axis and heading torward it with a velocity in
        the +x direction: has a sphere shaped PhysX Collider, a PhysX Rigid Body, and a Sphere Shape.
    
    Materials:
    Bounce - All three spheres and the Perface mesh area lined up with Sphere 1 have the Bounce material applied to
        them. This material interacts with other materials by bouncing with the restitution factor of an average of
        each entity that collides restitution value. Has restitution value: 1
    PartialBounce - The Perface mesh area lined up with Sphere 2 has the partial bounce material applied. This material
        interacts with other materials by responding with a restitution factor that is an average of the two materials
        that interact. Has restitution value: 0
    NoBounce - The Perface mesh area lined up with Sphere 0 have the NoBounce material. This material interacts with
        other materials by bouncing with the restitution factor of the material with the lowest restitution value.
        Has restitution value: 0

    Expected Behavior: Sphere 0 will not bounce, Sphere 1 will bounce away from the Perface Entity faster than
        Sphere 2 will.

    Test Steps:
    1) Open Level
    2) Enter Game Mode
    3) Create Entity objects
    4) Iterate through all entities and validate them
    5) Validate that Entities Exist
    6) Iterate through each of the three spheres and test their bounces
    7) Further evaluate that sphere entities exist
    8) Validate Initial Positions and Velocities
    9) Set up handler and wait for collision
    10) Get and Validate Final Positions and Velocities
    11) Log Results
    12) Validate Orientations and Final Velocities
    13) Exit Game Mode
    14) Close Editor

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
    import azlmbr.math as math

    # Constants
    FLOAT_THRESHOLD = sys.float_info.epsilon
    FINAL_VELOCITY_THRESHOLD = 0.01
    STATIONARY_SPHERE_THRESHOLD = 2
    TIMEOUT = 1.0

    # Helper Functions
    class Entity:
        def __init__(self, name, expected_initial_velocity=None, expected_final_velocity=None):
            self.id = general.find_game_entity(name)
            self.name = name
            self.EXPECTED_INITIAL_VELOCITY = expected_initial_velocity
            self.EXPECTED_FINAL_VELOCITY = expected_final_velocity
            self.initial_velocity = None
            self.final_velocity = None
            self.initial_position = None
            self.final_position = None
            self.collision_happened = False
            self.handler = None

        class Entity_Tests:
            found = None
            found_position = None
            found_velocity = None
            valid_init_velocity = None
            valid_final_velocity = None
            collision_happened = None

        def check_id(self):
            self.Entity_Tests.found = Tests.__dict__[self.name + "_found"]
            Report.critical_result(self.Entity_Tests.found, self.id.isValid())

        def activate_entity(self):
            Report.info("Activating Entity : " + self.name)
            azlmbr.entity.GameEntityContextRequestBus(azlmbr.bus.Broadcast, "ActivateGameEntity", self.id)

        def values_found(self):
            self.Entity_Tests.found_position = Tests.__dict__[self.name + "_position_found"]
            self.Entity_Tests.found_velocity = Tests.__dict__[self.name + "_velocity_found"]

            Report.critical_result(self.Entity_Tests.found_position, vector_valid(self.initial_position, False))
            Report.critical_result(self.Entity_Tests.found_velocity, vector_valid(self.initial_velocity, False))

        def perface_values_found(self):
            self.Entity_Tests.found_position = Tests.__dict__[self.name + "_position_found"]
            Report.critical_result(self.Entity_Tests.found_position, vector_valid(self.initial_position, False))

        def get_initial_position_and_velocity(self):
            self.initial_velocity = azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "GetLinearVelocity", self.id)
            self.initial_position = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", self.id)

        def get_final_position_and_velocity(self):
            self.final_velocity = azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "GetLinearVelocity", self.id)
            self.final_position = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", self.id)

        def validate_sphere_velocity(self):
            if self.collision_happened:
                velocity_valid = (
                    abs(self.final_velocity.x - self.EXPECTED_FINAL_VELOCITY.x) < FINAL_VELOCITY_THRESHOLD
                    and abs(self.final_velocity.y - self.EXPECTED_FINAL_VELOCITY.y) < FINAL_VELOCITY_THRESHOLD
                    and abs(self.final_velocity.z - self.EXPECTED_FINAL_VELOCITY.z) < FINAL_VELOCITY_THRESHOLD
                )
                self.Entity_Tests.valid_final_velocity = Tests.__dict__[self.name + "_final_velocity_valid"]
                Report.result(self.Entity_Tests.valid_final_velocity, velocity_valid)
            else:
                velocity_valid = (
                    abs(self.initial_velocity.x - self.EXPECTED_INITIAL_VELOCITY.x) < FLOAT_THRESHOLD
                    and abs(self.initial_velocity.y - self.EXPECTED_INITIAL_VELOCITY.y) < FLOAT_THRESHOLD
                    and abs(self.initial_velocity.z - self.EXPECTED_INITIAL_VELOCITY.z) < FLOAT_THRESHOLD
                )
                self.Entity_Tests.valid_init_velocity = Tests.__dict__[self.name + "_velocity_valid"]
                Report.critical_result(self.Entity_Tests.valid_init_velocity, velocity_valid)

        def on_collision_begin(self, args):
            if self.id.equal(args[0]):
                self.collision_happened = True

        def set_handler(self, id):
            self.handler = azlmbr.physics.CollisionNotificationBusHandler()
            self.handler.connect(id)
            self.handler.add_callback("OnCollisionBegin", self.on_collision_begin)

    def report_sphere_values(entity):
        Report.info_vector3(entity.initial_position, "{} initial position: ".format(entity.name))
        Report.info_vector3(entity.initial_velocity, "{} initial velocity: ".format(entity.name))

        Report.info_vector3(entity.final_position, "{} final position: ".format(entity.name))
        Report.info_vector3(entity.final_velocity, "{} final velocity: ".format(entity.name))

    def report_perface_values(entity):
        Report.info_vector3(entity.initial_position, "{} initial position: ".format(entity.name))

    def validate_positions():
        # Initial orientation is confirmed by z axis values, if there are further issues a collision
        # will not as expected.
        Report.info("Checking Initial Orientation")
        initial_orientaition = (
            sphere_0.initial_position.z
            == sphere_1.initial_position.z
            == sphere_2.initial_position.z
            == perface_entity.initial_position.z
        )
        Report.result(Tests.initial_orientaition_valid, initial_orientaition)
        # Final orientation is confirmed if Sphere 0 stopped next to the Perface Entity.
        Report.info("Checking Final Orientation")
        final_orientation = (
            abs(perface_entity.final_position.x - sphere_0.final_position.x) < FLOAT_THRESHOLD
            and abs(perface_entity.final_position.z - sphere_0.final_position.z) < FLOAT_THRESHOLD
            and abs(perface_entity.final_position.y - sphere_0.final_position.y) < STATIONARY_SPHERE_THRESHOLD
        )

        Report.result(Tests.final_orientation_valid, final_orientation)

    def vector_valid(vector, can_be_zero):
        if can_be_zero:
            return vector != None
        else:
            return vector != None and not vector.IsZero()

    # Main Script
    helper.init_idle()
    # 1) Open Level
    helper.open_level("Physics", "Material_PerFaceMaterialGetsCorrectMaterial")

    # 2) Enter Game Mode
    helper.enter_game_mode(Tests.enter_game_mode)

    # 3) Create entity objects
    sphere_0 = Entity("Sphere_0", math.Vector3(0.0, -10.0, 0.0), math.Vector3(0.0, 0.0, 0.0))
    sphere_1 = Entity("Sphere_1", math.Vector3(-10.0, 0.0, 0.0), math.Vector3(10.09, 1.85, 1.18))
    sphere_2 = Entity("Sphere_2", math.Vector3(10.0, 0.0, 0.0), math.Vector3(-5.0, 0.0, 0.0))
    perface_entity = Entity("Perface_Entity")
    entity_list = [sphere_0, sphere_1, sphere_2, perface_entity]
    spheres = [sphere_0, sphere_1, sphere_2]

    # 4) Iterate through all entities and validate them
    for entity in entity_list:
        # 5) Validate that Entities Exist
        entity.check_id()
    # Extra steps for Perface Entity as it will no longer be iterated
    perface_entity.get_initial_position_and_velocity()
    perface_entity.perface_values_found()
    perface_entity.get_final_position_and_velocity()
    report_perface_values(perface_entity)

    # 6) Iterate through each of the three spheres and test their bounces
    for entity in spheres:
        # 7) Further evaluate that sphere entities exist
        entity.activate_entity()
        entity.get_initial_position_and_velocity()
        
        # 8) Validate Initial Positions and Velocities
        entity.values_found()
        entity.validate_sphere_velocity()

        # 9) Set up handler and wait for collision
        entity.set_handler(perface_entity.id)

        # Wait for collision
        helper.wait_for_condition(lambda: entity.collision_happened, TIMEOUT)
        # Report Collision
        entity.Entity_Tests.collision_happened = Tests.__dict__[entity.name + "_collided_with_perface"]
        Report.result(entity.Entity_Tests.collision_happened, entity.collision_happened)

        # 10) Get and Validate Final Positions and Velocities
        entity.get_final_position_and_velocity()
        entity.validate_sphere_velocity()

        # 11) Log Results
        report_sphere_values(entity)

    # 12) Validate Orientations and Final Velocities
    validate_positions()
    Report.result(
        Tests.speed_comparision, sphere_1.final_velocity.GetLength() > sphere_2.final_velocity.GetLength()
    )

    # 13) Exit Game Mode
    helper.exit_game_mode(Tests.exit_game_mode)



if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Material_PerFaceMaterialGetsCorrectMaterial)
