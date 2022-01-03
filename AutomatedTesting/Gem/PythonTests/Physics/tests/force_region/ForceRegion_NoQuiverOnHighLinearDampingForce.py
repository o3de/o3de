"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# Test case ID : C15845879
# Test Case Title : Check that linear damping with high values do not make the object to quiver


# fmt: off
class Tests:
    enter_game_mode                 = ("Entered game mode",               "Failed to enter game mode")
    exit_game_mode                  = ("Exited game mode",                "Couldn't exit game mode")
    sphere_found                    = ("Found sphere",                    "Did not find sphere")
    force_region_found              = ("Found force region",              "Did not find force region")
    check_relative_position         = ("Sphere is above force region",    "Sphere isn't above force region")
    sphere_moving_down              = ("Sphere heading to force region",  "Sphere has invalid initial velocity")
    sphere_entered_force_region     = ("Sphere has entered force region", "Sphere never entered force region")
    sphere_stopped_moving           = ("Sphere final velocity is zero",   "Sphere final velocity invalid")
    sphere_still_above_force_region = ("Sphere still above force region", "Sphere not above force region")
    no_quiver                       = ("Sphere is not quivering",         "Sphere quivering in force region")
# fmt: on


def ForceRegion_NoQuiverOnHighLinearDampingForce():
    """
    Summary: Check that linear damping with high values do not make the object to quiver

    Level Description:
    sphere - Starts above the force_region entity with initial velocity in the negative z direction and
    gravity disabbled; has physx collider in sphere shape, physx rigid body, and sphere shape
    force_region - Sits below sphere entity, has linear damping force set at 100 and region has scaling
        (5,5,5); has physx collider in box shape and physx force region

    Expected Behavior: Sphere falls into force region and is stuck by the damping force. It specifically should
        not quiver up and down.

    Test Steps:
    1) Open Level
    2) Enter Game Mode
    3) Create and Validate Entities
    4) Setup handler and wait for sphere to enter force region
    5) Validate the Sphere remains in Force Region
    6) Check to see if the sphere is quivering
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
    VELOCITY_THRESHOLD = 0.01
    QUIVER_THRESHOLD = 0.01
    SLOWDOWN_FRAMES = 30
    SPHERE_STOP_OFFSET = 3.5

    # Helper Functions
    class Entity:
        def __init__(self, name):
            self.id = general.find_game_entity(name)
            self.name = name
            self.force_region_id = None
            self.entered_force_region = False
            self.quiver_reference = None
            Report.critical_result(Tests.__dict__[self.name + "_found"], self.id.isValid())

        @property
        def position(self):
            # type () -> Vector3
            return azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", self.id)

        @property
        def velocity(self):
            # type () -> Vector3
            return azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "GetLinearVelocity", self.id)

        @property
        def is_moving_up(self):
            # type () -> bool
            return (
                abs(self.velocity.x) < FLOAT_THRESHOLD
                and abs(self.velocity.y) < FLOAT_THRESHOLD
                and self.velocity.z > 0.0
            )

        def set_handler(self):
            self.handler = azlmbr.physics.ForceRegionNotificationBusHandler()
            self.handler.connect(None)
            self.handler.add_callback("OnCalculateNetForce", self.on_calculate_net_force)

        def on_calculate_net_force(self, args):
            # type (list) -> None
            # Flips the collision happened boolean for the sphere object and prints the force values.
            if self.force_region_id.Equal(args[0]) and self.id.Equal(args[1]) and not self.entered_force_region:
                self.entered_force_region = True

    def sphere_not_quivering():
        # type () -> bool
        # Returns False if sphere "quivers" from its initial position, True if it stays close to it's original position
        return abs(sphere.position.z - sphere.quiver_reference) > QUIVER_THRESHOLD

    def sphere_above_force_region(sphere_position, force_region_position):
        # type () -> bool
        return (
            abs(sphere_position.x - force_region_position.x) < FLOAT_THRESHOLD
            and abs(sphere_position.y - force_region_position.y) < FLOAT_THRESHOLD
            and sphere_position.z > force_region_position.z
        )

    # Main Script
    helper.init_idle()
    # 1) Open Level
    helper.open_level("Physics", "ForceRegion_NoQuiverOnHighLinearDampingForce")

    # 2) Enter Game Mode
    helper.enter_game_mode(Tests.enter_game_mode)

    # 3) Create and Validate Entities
    sphere = Entity("sphere")
    force_region = Entity("force_region")

    sphere.force_region_id = force_region.id
    Report.critical_result(Tests.sphere_moving_down, not sphere.is_moving_up)
    Report.critical_result(
        Tests.check_relative_position, sphere_above_force_region(sphere.position, force_region.position)
    )

    # 4) Setup handler and wait for sphere to enter force region
    sphere.set_handler()
    Report.critical_result(
        Tests.sphere_entered_force_region, helper.wait_for_condition(lambda: sphere.entered_force_region, TIMEOUT)
    )

    # 5) Validate the Sphere remains in Force Region
    # Must wait for the sphere to slow down
    Report.result(Tests.sphere_stopped_moving, helper.wait_for_condition(lambda: sphere.velocity.IsZero(VELOCITY_THRESHOLD), TIMEOUT))
    # Force region has scaling (5,5,5). Thus the upper edge of the force region is 2.5m above the transform. With proper offset we can
    # see that sphere is stuck on top of the force region and did not bounce off. 
    Report.result(Tests.sphere_still_above_force_region, (sphere.position.z - force_region.position.z) < SPHERE_STOP_OFFSET)

    # 6) Check to see if the sphere is quivering
    sphere.quiver_reference = sphere.position.z
    Report.result(Tests.no_quiver, not helper.wait_for_condition(sphere_not_quivering, TIMEOUT))

    # 7) Exit Game Mode
    helper.exit_game_mode(Tests.exit_game_mode)



if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(ForceRegion_NoQuiverOnHighLinearDampingForce)
