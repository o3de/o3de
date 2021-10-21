"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# Test case ID : C5932042
# Test Case Title : Check that force region exerts linear damping force on rigid bodies



# fmt: off
class Tests:
    enter_game_mode         = ("Entered game mode",                         "Failed to enter game mode")
    sphere_validated        = ("Sphere entity validated",                   "Sphere entity NOT validated")
    force_region_validated  = ("Force Region validated",                    "Force Region NOT validated")
    trigger_validated       = ("Trigger entity validated",                  "Trigger entity NOT validated")
    sphere_pos_found        = ("Sphere position found",                     "Sphere position NOT found")
    force_region_pos_found  = ("Force Region position found",               "Force Region position NOT found")
    trigger_pos_found       = ("Trigger position found",                    "Trigger position NOT found")
    level_setup             = ("Level looks set up right",                  "Level NOT set up right")
    damping_force_entered   = ("Sphere entered linear damping region",      "Linear damping region never entered")
    damping_force_expected  = ("Damping force was as expected",             "Damping force differed from expected")
    sphere_slowed           = ("Sphere slowed down",                        "Sphere DID NOT slow down")
    sphere_stopped          = ("Sphere entity stopped",                     "Sphere entity DID NOT stop")
    force_region_no_move    = ("Force Region did not move",                 "Fore Region DID move")
    trigger_no_move         = ("Trigger did not move",                      "Trigger DID move")
    timed_out               = ("The test did not time out",                 "The test TIMED OUT")
    trigger_not_triggered   = ("The Trigger was not triggered",             "The Trigger WAS triggered")
    exit_game_mode          = ("Exited game mode",                          "Couldn't exit game mode")

# fmt: on


def ForceRegion_LinearDampingForceOnRigidBodies():
    # type: () -> None
    """
    Summary:
    Runs an automated test to ensure linear damping is exerted on rigid body objects from force regions.

    Level Description:
    A sphere (entity: Sphere) in positioned above a large cube force region (entity: force_region_entity) who is
    assigned a linear damping force with damping of 10.0. The Sphere has gravity enabled, and is positioned high
    enough for gravity to accelerate it faster than the maximum velocity inside the linear damping force region.

    Expected Behavior:
    When game mode is entered, gravity should accelerate the Sphere downward. The velocity of the Sphere should peak
    right before it enters force_region_entity. Upon entering the force region, the Sphere should noticeably slow
    down. The slower velocity should have a substantially larger (less negative) velocity.

    Test Steps:
    0) Define useful classes and constants
    1) Loads the level / Enters game mode
    2) Retrieve and validate entities
    3) Ensures that the test object (Sphere) is located
        3.5) Set up event handlers
    4) Execute test until exit condition met
    5) Logs results
        5.5) Dump all collected data to log
    6) Close the editor

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
    import azlmbr.math as azmath

    # Entity base class: Handles basic entity data
    class EntityBase:
        def __init__(self, name):
            self.name = name
            self.id = None
            self.initial_pos = None
            self.current_pos = None

    # Specific Sphere class
    class Sphere(EntityBase):
        def __init__(self, name):
            EntityBase.__init__(self, name)
            self.initial_velocity = None
            self.initial_velocity_magnitude = None
            self.current_velocity = None
            self.slowed = False
            self.stopped = False

        def check_for_stop(self):
            if not self.stopped:
                self.current_velocity = azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "GetLinearVelocity", self.id)
                self.slowed = self.initial_velocity_magnitude > self.current_velocity.GetLength() + (
                    0.5 * self.initial_velocity_magnitude
                )
                self.stopped = self.current_velocity.IsZero()
            return self.stopped

    # Specific Force Region class
    class ForceRegion(EntityBase):
        def __init__(self, name):
            EntityBase.__init__(self, name)
            self.entered = False
            self.object_entered = None
            self.expected_force_direction = None
            self.actual_force_vector = None
            self.actual_force_magnitude = None
            self.handler = None

    # Specific Trigger class
    class Trigger(EntityBase):
        def __init__(self, name):
            EntityBase.__init__(self, name)
            self.triggered = False
            self.triggering_obj = None
            self.handler = None

    # Constants
    CLOSE_ENOUGH = 0.001
    TIME_OUT = 10.0
    INITIAL_VELOCITY = azmath.Vector3(0.0, 0.0, -10.0)

    # 1) Open level / Enter game mode
    helper.init_idle()
    helper.open_level("Physics", "ForceRegion_LinearDampingForceOnRigidBodies")
    helper.enter_game_mode(Tests.enter_game_mode)

    # 2) Retrieve and validate entities
    sphere = Sphere("Sphere")
    sphere.id = general.find_game_entity(sphere.name)
    force_region = ForceRegion("ForceRegion")
    force_region.id = general.find_game_entity(force_region.name)
    trigger = Trigger("Trigger")
    trigger.id = general.find_game_entity(trigger.name)

    Report.critical_result(Tests.sphere_validated, sphere.id.IsValid())
    Report.critical_result(Tests.force_region_validated, force_region.id.IsValid())
    Report.critical_result(Tests.trigger_validated, trigger.id.IsValid())

    # 3) Log Entities' positions and initial data
    sphere.initial_pos = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", sphere.id)
    force_region.initial_pos = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", force_region.id)
    trigger.initial_pos = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", trigger.id)

    Report.critical_result(Tests.sphere_pos_found, sphere.initial_pos is not None and not sphere.initial_pos.IsZero())
    Report.critical_result(
        Tests.force_region_pos_found, force_region.initial_pos is not None and not force_region.initial_pos.IsZero()
    )
    Report.critical_result(
        Tests.trigger_pos_found, trigger.initial_pos is not None and not trigger.initial_pos.IsZero()
    )

    sphere.initial_velocity = azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "GetLinearVelocity", sphere.id)

    level_correct = (
        (abs(sphere.initial_pos.y - force_region.initial_pos.y) < CLOSE_ENOUGH)
        and (abs(sphere.initial_pos.y - trigger.initial_pos.y) < CLOSE_ENOUGH)
        and (abs(sphere.initial_pos.x - force_region.initial_pos.x) < CLOSE_ENOUGH)
        and (abs(sphere.initial_pos.x - trigger.initial_pos.x) < CLOSE_ENOUGH)
        and (sphere.initial_pos.z > force_region.initial_pos.z > trigger.initial_pos.z)
        and sphere.initial_velocity.IsClose(INITIAL_VELOCITY, CLOSE_ENOUGH)
    )

    Report.critical_result(Tests.level_setup, level_correct)

    sphere.current_pos = sphere.initial_pos
    force_region.current_pos = force_region.initial_pos
    trigger.current_pos = trigger.initial_pos
    force_region.expected_force_direction = sphere.initial_velocity.MultiplyFloat(-1.0)
    force_region.expected_force_direction.Normalize()
    sphere.current_velocity = sphere.initial_velocity
    sphere.initial_velocity_magnitude = sphere.initial_velocity.GetLength()

    # 3.5) Set up variables and handler for observing force region interaction

    def done_collecting_results():

        # Update current positions
        sphere.current_pos = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", sphere.id)
        force_region.current_pos = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", force_region.id)
        trigger.current_pos = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", trigger.id)

        return force_region.entered and sphere.check_for_stop()

    # Force Region Event Handler
    def on_calc_net_force(args):
        if args[0].Equal(force_region.id):
            if args[1].Equal(sphere.id):
                if not force_region.entered:
                    force_region.entered = True
                    force_region.object_entered = sphere
                    force_region.actual_force_vector = args[2]
                    force_region.actual_force_magnitude = args[3]
                    Report.info("Entity: {} entered entity: {}'s volume".format(sphere.name, force_region.name))

    def on_trigger_entered(args):
        if args[0].Equal(sphere.id):
            trigger.triggered = True
            trigger.triggering_obj = sphere

    # Assign event handlers
    force_region.handler = azlmbr.physics.ForceRegionNotificationBusHandler()
    force_region.handler.connect(None)
    force_region.handler.add_callback("OnCalculateNetForce", on_calc_net_force)

    trigger.handler = azlmbr.physics.TriggerNotificationBusHandler()
    trigger.handler.connect(trigger.id)
    trigger.handler.add_callback("OnTriggerEnter", on_trigger_entered)

    # 4) Execute test until exit condition is met
    Report.critical_result(Tests.timed_out, helper.wait_for_condition(done_collecting_results, TIME_OUT))

    # 5) Log results
    Report.result(Tests.damping_force_entered, force_region.entered)
    Report.result(
        Tests.damping_force_expected,
        force_region.actual_force_vector.IsClose(force_region.expected_force_direction, CLOSE_ENOUGH),
    )
    Report.result(Tests.sphere_slowed, sphere.slowed)
    Report.result(Tests.sphere_stopped, sphere.stopped)
    Report.result(Tests.trigger_not_triggered, not trigger.triggered)
    Report.result(Tests.force_region_no_move, force_region.initial_pos.IsClose(force_region.current_pos, CLOSE_ENOUGH))
    Report.result(Tests.trigger_no_move, trigger.initial_pos.IsClose(trigger.current_pos, CLOSE_ENOUGH))

    # 5.5) Collected Data Dump
    Report.info(" ********** Collected Data ***************")
    Report.info("{}:".format(sphere.name))
    Report.info_vector3(sphere.initial_pos, " Initial position:")
    Report.info_vector3(sphere.current_pos, " Final position:")
    Report.info_vector3(sphere.initial_velocity, " Initial velocity:")
    Report.info_vector3(sphere.current_velocity, " Final velocity:")
    Report.info(" Slowed: {}".format(sphere.slowed))
    Report.info(" Stopped: {}".format(sphere.stopped))
    Report.info("***********************************")
    Report.info("{}:".format(force_region.name))
    Report.info_vector3(force_region.initial_pos, " Initial position:")
    Report.info_vector3(force_region.current_pos, " Final position:")
    Report.info_vector3(force_region.expected_force_direction, " Expected Force Direction:")
    Report.info_vector3(
        force_region.actual_force_vector, " Actual Force Direction:", force_region.actual_force_magnitude
    )
    Report.info(" Entered: {}".format(force_region.entered))
    Report.info(" Object Entered: {}".format(force_region.object_entered.name))
    Report.info("***********************************")
    Report.info("{}:".format(trigger.name))
    Report.info_vector3(trigger.initial_pos, " Initial position:")
    Report.info_vector3(trigger.current_pos, " Final position:")
    Report.info(" Triggered: {}".format(trigger.triggered))
    Report.info(" Triggering Object: {}".format(trigger.triggering_obj))
    Report.info("***********************************")

    # 6) Exit Game mode
    helper.exit_game_mode(Tests.exit_game_mode)
    Report.info("*** FINISHED TEST ***")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(ForceRegion_LinearDampingForceOnRigidBodies)
