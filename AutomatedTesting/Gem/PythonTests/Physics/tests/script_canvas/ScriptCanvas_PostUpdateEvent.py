"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# Test case ID : C14195074
# Test Case Title : Verify Postsimulate Events


# fmt: off
class Tests:
    enter_game_mode       = ("Entered game mode",                   "Failed to enter game mode")
    exit_game_mode        = ("Exited game mode",                    "Failed to exit game mode")
    lead_sphere_found     = ("Lead_Sphere is valid",                "Lead_Sphere is not valid")
    follow_sphere_found   = ("Follow_Sphere is valid",              "Follow_Sphere is not valid")
    no_movement           = ("Spheres start with no movement",      "Spheres not stationary")
    initial_position      = ("Initial position is valid",           "Initial position is not valid")
    lead_sphere_velocity  = ("Lead_Sphere has valid velocity",      "Lead_Sphere velocity not valid")
    spheres_moving        = ("Spheres have both moved",             "Both sphere have not moved before timeout")
    follow_condition_true = ("Follow_Sphere is correctly trailing", "Follow_Sphere follow distance not valid")
# fmt: on


def ScriptCanvas_PostUpdateEvent():
    """
    Summary: Verifies that Postsimulate Event node in Script Canvas works as expected.

    Level Description:
    Lead_Sphere - Directly next to Follow_sphere on the +x axis; has rigid body(gravity disabled), sphere shape
        collider, sphere shape
    Follow_Sphere - Directly next to Lead_Sphere on the -x axis; has rigid body (gravity disabled, kinematic
        enabled), sphere shape collider, sphere shape, script canvas

    Script Canvas - After every PhysX frame is calculated Follow_Sphere is moved to directly next to the 
        Lead_sphere before being presented in the viewer
    PhysX Configuration - The PhysX configuration file was modified to lower the frame rate to 20 Hz for visual debug

    Expected Behavior: Follow_Sphere follows Lead_spheres position, staying within direct contact no matter the speed

    Test Steps:
    1) Open Level
    2) Enter Game Mode
    3) Create and Validate entities
    4) Validate spheres are not moving
    5) Check Position of Spheres
    6) Start moving sphere and check that it acts correctly
    7) Wait until Lead_Sphere has moved a set distance
    8) Verify Follow_Sphere follow distance
    9) Log results
    10) Exit Game Mode
    11) Close Editor

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

    import azlmbr
    import azlmbr.legacy.general as general
    import azlmbr.bus
    import azlmbr.math as math

    # Constants
    FLOAT_THRESHOLD = sys.float_info.epsilon
    TIMEOUT = 5
    INITIAL_OFFSET = 1
    REQUIRED_MOVEMENT = 2
    LEAD_SPHERE_VELOCITY = 10.0
    FINAL_OFFSET = INITIAL_OFFSET

    # Helper Functions
    class Entity:
        def __init__(self, name):
            self.id = general.find_game_entity(name)
            self.name = name

            self.initial_position = self.position
            self.final_position = None
            # Validate Entities
            found = Tests.__dict__[self.name.lower() + "_found"]
            Report.critical_result(found, self.id.isValid())

        @property
        def position(self):
            return azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", self.id)
        
        @property
        def velocity(self):
                return azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "GetLinearVelocity", self.id)

        def set_velocity(self, x_velocity, y_velocity, z_velocity):
            velocity = math.Vector3(x_velocity, y_velocity, z_velocity)
            azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "SetLinearVelocity", self.id, velocity)

        def moved_enough(self):
            current_position = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", self.id)
            return abs(self.initial_position.x - current_position.x) >= REQUIRED_MOVEMENT

        def report_values(self):
            Report.info_vector3(self.initial_position, "{} initial position: ".format(self.name))
            Report.info_vector3(self.final_position, "{} final position: ".format(self.name))


    def check_relative_position(lead_sphere_position, follow_sphere_position, offset):
        return (
            abs((lead_sphere_position.x - follow_sphere_position.x) - offset) < FLOAT_THRESHOLD
            and abs(lead_sphere_position.y - follow_sphere_position.y) < FLOAT_THRESHOLD
            and abs(lead_sphere_position.z - follow_sphere_position.z) < FLOAT_THRESHOLD
        )

    def velocity_zero(sphere_velocity):
        return (
            abs(sphere_velocity.x) < FLOAT_THRESHOLD
            and abs(sphere_velocity.y) < FLOAT_THRESHOLD
            and abs(sphere_velocity.z) < FLOAT_THRESHOLD
        )

    def velocity_valid(lead_sphere_velocity):
        return (
            lead_sphere_velocity.x == LEAD_SPHERE_VELOCITY
            and abs(lead_sphere_velocity.y) < FLOAT_THRESHOLD
            and abs(lead_sphere_velocity.z) < FLOAT_THRESHOLD
        )

    # Main Script
    helper.init_idle()
    # 1) Open Level
    helper.open_level("Physics", "ScriptCanvas_PostUpdateEvent")

    # 2) Enter Game Mode
    helper.enter_game_mode(Tests.enter_game_mode)

    # 3) Create and validate entities
    lead_sphere = Entity("Lead_Sphere")
    follow_sphere = Entity("Follow_Sphere")

    # 4) Validate Spheres are not moving
    Report.critical_result(
        Tests.no_movement, velocity_zero(lead_sphere.velocity) and velocity_zero(follow_sphere.velocity)
    )

    # 5) Check Position of Spheres
    Report.result(
        Tests.initial_position,
        check_relative_position(lead_sphere.initial_position, follow_sphere.initial_position, INITIAL_OFFSET),
    )

    # 6) Start moving sphere and check that it acts correctly
    lead_sphere.set_velocity(LEAD_SPHERE_VELOCITY, 0.0, 0.0)
    Report.result(Tests.lead_sphere_velocity, velocity_valid(lead_sphere.velocity))

    # 7) Wait until Lead_Sphere has moved a set distance
    Report.result(Tests.spheres_moving, helper.wait_for_condition(lead_sphere.moved_enough, TIMEOUT))

    # 8) Verify Follow_Sphere follow distance
    lead_sphere.final_position = lead_sphere.position
    follow_sphere.final_position = follow_sphere.position
    Report.result(
        Tests.follow_condition_true, check_relative_position(lead_sphere.final_position, follow_sphere.final_position, FINAL_OFFSET)
    )

    # 9) Log results
    lead_sphere.report_values()
    follow_sphere.report_values()

    # 10) Exit Game Mode
    helper.exit_game_mode(Tests.exit_game_mode)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(ScriptCanvas_PostUpdateEvent)
