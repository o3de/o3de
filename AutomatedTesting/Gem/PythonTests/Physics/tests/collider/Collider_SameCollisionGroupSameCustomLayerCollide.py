"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# Test case ID : C4976244
# Test Case Title : Checks that two entities of similar custom layer collide



# fmt: off
class Tests:
    enter_game_mode                   = ("Entered game mode",                 "Failed to enter game mode")
    moving_sphere_found               = ("Moving sphere found",               "Moving sphere not found")
    stationary_sphere_found           = ("Stationary sphere found",           "Stationary sphere not found")
    velocities_before_collision_valid = ("Sphere velocities are valid",       "Sphere velocities are not valid")
    orientation_before_collision      = ("Both spheres are aligned properly", "Spheres are not aligned properly")
    spheres_collided                  = ("Collision was detected",            "A collision was not detected")
    orientation_after_collision       = ("Spheres are aligned properly",      "Spheres are not aligned properly")
    velocities_after_collision_valid  = ("Velocity after collision valid",    "Velocity after collision not valid")
    exit_game_mode                    = ("Exited game mode",                  "Couldn't exit game mode")

# fmt: on


def Collider_SameCollisionGroupSameCustomLayerCollide():
    # type: () -> None
    """
    Summary:
    Runs an automated test to ensure to rigid bodies on similar collision layer and group collide.

    Level Description:
    Moving Sphere (Entity) - On the same x axis as the Stationary Sphere, moving in the positive x direction;
        has sphere shaped PhysX Collider, PhysX Rigid Body, Sphere Shape. Collision Group All, layer A
    Stationary Sphere (Entity) - On the same x axis as the Moving Sphere;
        has sphere shaped PhysX Collider, PhysX Rigid Body, Sphere Shape Collision Group All, layer A

    Expected Behavior: The moving sphere will move torward the stationary sphere in the positive x direction
        and collide with it. Both spheres will then separate and move in opposite directions.

    Test Steps:
    1) Load the level
    2) Enter Game Mode
    3) Validate entities
    4) Validate positions and velocities
    5) Start handlers
    6) Wait for collision
    7) Validated and logs results
    8) Exit Game Mode
    9) Close the editor

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

    # Helper functions
    # Callback function for the collision handler
    def on_collision_begin(args):
        #  type (list) -> None
        Report.info("Collision Occurred")
        other_id = args[0]
        if other_id.Equal(moving_sphere.id):
            stationary_sphere.collision_happened = True

    class Entity:
        def __init__(self, name):
            self.id = general.find_game_entity(name)
            self.name = name
            self.initial_velocity = azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "GetLinearVelocity", self.id)
            self.final_velocity = None
            self.initial_position = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", self.id)
            self.final_position = None
            self.collision_happened = False
            Report.info_vector3(self.initial_position, "{} initial position: ".format(self.name))
            Report.info_vector3(self.initial_velocity, "{} initial velocity: ".format(self.name))

        def get_final_position_and_velocity(self):
            # type () -> None
            self.final_velocity = azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "GetLinearVelocity", self.id)
            self.final_position = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", self.id)

        def report_final_values(self):
            # type () -> None
            Report.info_vector3(self.final_position, "{} final position: ".format(self.name))
            Report.info_vector3(self.final_velocity, "{} final velocity: ".format(self.name))

        def moving_in_x_direction(self, positive_x_direction):
            # type (bool) -> bool
            velocity_vector = azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "GetLinearVelocity", self.id)
            
            if positive_x_direction:
                correct_direction = velocity_vector.x > 0
            else:
                correct_direction = velocity_vector.x < 0
            return (
                correct_direction and abs(velocity_vector.y) < FLOAT_THRESHOLD and abs(velocity_vector.z) < FLOAT_THRESHOLD
            )
    
    # Checks if spheres are in the correct orientation
    def validate_positions(moving_entity_position, stationary_entity_position):
        # type (Vector3, Vector3) -> bool
        return (
            abs(moving_entity_position.z - stationary_entity_position.z) < FLOAT_THRESHOLD
            and abs(moving_entity_position.y - stationary_entity_position.y) < FLOAT_THRESHOLD
            and moving_entity_position.x < stationary_entity_position.x
        )

    # Main Script
    # 1) Load the level
    helper.init_idle()
    helper.open_level("physics", "Collider_SameCollisionGroupSameCustomLayerCollide")

    # 2) Enter Game Mode
    helper.enter_game_mode(Tests.enter_game_mode)

    # 3) Validate entities
    moving_sphere = Entity("Moving_Sphere")
    stationary_sphere = Entity("Stationary_Sphere")

    Report.critical_result(Tests.moving_sphere_found, moving_sphere.id.isValid())
    Report.critical_result(Tests.stationary_sphere_found, stationary_sphere.id.isValid())

    # 4) Validate positions and velocities
    Report.critical_result(
        Tests.orientation_before_collision,
        validate_positions(moving_sphere.initial_position, stationary_sphere.initial_position),
    )
    Report.critical_result(
        Tests.velocities_before_collision_valid,
        moving_sphere.moving_in_x_direction(positive_x_direction = True)
        and stationary_sphere.initial_velocity.IsZero(FLOAT_THRESHOLD),
    )

    # 5) Start handler
    handler = azlmbr.physics.CollisionNotificationBusHandler()
    handler.connect(stationary_sphere.id)
    handler.add_callback("OnCollisionBegin", on_collision_begin)

    # 6) Wait for collision
    helper.wait_for_condition(lambda: stationary_sphere.collision_happened, TIMEOUT)

    # 7) Validated and logs results
    Report.result(Tests.spheres_collided, stationary_sphere.collision_happened)
    moving_sphere.get_final_position_and_velocity()
    stationary_sphere.get_final_position_and_velocity()

    Report.result(
        Tests.orientation_after_collision,
        validate_positions(moving_sphere.final_position, stationary_sphere.final_position),
    )
    Report.result(
        Tests.velocities_after_collision_valid,
        moving_sphere.moving_in_x_direction(positive_x_direction = False) and stationary_sphere.moving_in_x_direction(positive_x_direction = True)
    )

    moving_sphere.report_final_values()
    stationary_sphere.report_final_values()

    # 8) Exit Game Mode
    helper.exit_game_mode(Tests.exit_game_mode)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Collider_SameCollisionGroupSameCustomLayerCollide)
