"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# Test case ID : C4976199
# Test Case Title : Verify that with higher linear damping, the object in motion comes to rest faster


# fmt: off
class Tests:
    enter_game_mode_low      = ("Entered game mode low",                                 "Failed to enter game mode low")
    enter_game_mode_medium   = ("Entered game mode medium",                              "Failed to enter game medium")
    enter_game_mode_high     = ("Entered game mode high",                                "Failed to enter game mode high")
    find_sphere_low          = ("Find sphere low",                                       "Failed to find sphere low")
    find_sphere_medium       = ("Find sphere medium",                                    "Failed to find sphere medium")
    find_sphere_high         = ("Find sphere high",                                      "Failed to find sphere high")
    find_triggers_low        = ("Find triggers low",                                     "Failed to find triggers low")
    find_triggers_medium     = ("Find triggers medium",                                  "Failed to find triggers medium")
    find_triggers_high       = ("Find triggers high",                                    "Failed to find triggers high")
    x_movement               = ("x direction movement decreases with increased damping", "x direction movement does not decrease with increased damping")
    high_damping_no_movement = ("High damping IsClose to 0 movement",                    "High damping is not IsClose to 0 movement")
    triggers_tripped_low     = ("Low damping trips all 3 triggers",                      "Low damping did not trip all 3 triggers")
    triggers_tripped_medium  = ("Medium damping trips 2/3 triggers",                     "Medium damping does not trip 2/3 triggers")
    triggers_tripped_high    = ("High damping trips no triggers",                        "High damping trips triggers and should not")
    timeout                  = ("All spheres velocity IsClose to 0 before timeout",      "All spheres velocity are not IsClose to 0 before timeout")
    exit_game_mode_low       = ("Exited game mode low",                                  "Couldn't exit game mode low")
    exit_game_mode_medium    = ("Exited game mode medium",                               "Couldn't exit game mode medium")
    exit_game_mode_high      = ("Exited game mode high",                                 "Couldn't exit game mode high")
# fmt: on


def RigidBody_LinearDampingAffectsMotion():
    """
    The level consists of a PhysX Terrain component that is not interacted with (per the test case)
    and a PhysX collider with shape sphere, PhysX rigid bodies physics, and mesh with shape sphere.
    3 colliders with trigger are added to ensure x movement length is decreased quantitatively not
    just comparatively.
    The sphere starts asleep.
    We will enter game mode for each state defined

    1) Open level
    2) Enter game mode
    3) Set sphere attributes, measure initial position, and then ForceAwake
    5) Measure
    6) Exit game mode
    7) Run and repeat steps 2-6
    8) Report results

    Notes: 
    Initially we calculated time to stop by calling time.time() both when awakening the sphere and when velocity equaled zero.
    Comparing the time to stop accross sphere settings proved flaky.
    """
    # Setup path
    import os
    import sys
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    import azlmbr.legacy.general as general
    import azlmbr.bus
    import azlmbr.math as lymath

    class SphereInfo:
        def __init__(
            self,
            initial_linear_velocity,
            linear_damping,
            enter_game_mode_test,
            find_sphere_test,
            exit_game_mode_test,
            triggers_test,
        ):
            self.initial_linear_velocity = initial_linear_velocity
            self.linear_damping = linear_damping
            self.entity = None
            self.initial_position = None
            self.final_position = None
            self.timeout = False
            self.triggers = []
            self.enter_game_mode_test = enter_game_mode_test
            self.find_sphere_test = find_sphere_test
            self.exit_game_mode_test = exit_game_mode_test
            self.triggers_test = triggers_test

        def __str__(self):
            return """
            initial_linear_velocity = {}
            linear_damping = {}
            timeout = {}
            """.format(
                self.initial_linear_velocity.GetLength(), self.linear_damping, self.timeout
            )

        def report(self):
            Report.info(self.__str__())
            Report.info_vector3(self.initial_position, "Initial position")
            Report.info_vector3(self.final_position, "Final position")
            for trigger in self.triggers:
                Report.info(trigger.__str__())

    class Trigger:
        def __init__(self, name):
            self.name = name
            self.entity = None
            self.handler = None
            self.triggering_entity = None
            self.triggered = False

        def on_trigger(self, args):
            self.triggered = True
            self.triggering_entity = args[0]
            Report.info("{} was triggered by {}".format(self.name, self.triggering_entity_name()))

        def __str__(self):
            return """
            name = {}
            triggering entity name = {}
            triggered = {}
            """.format(
                self.name, self.triggering_entity_name(), self.triggered
            )

        def triggering_entity_name(self):
            if self.triggering_entity:
                return azlmbr.entity.GameEntityContextRequestBus(azlmbr.bus.Broadcast, "GetEntityName", self.triggering_entity)
            return None

    INITIAL_LINEAR_VELOCITY = lymath.Vector3(5.0, 0.0, 0.0)
    ZERO_VELOCITY = lymath.Vector3(0.0, 0.0, 0.0)
    TOLERANCE = 0.001
    TIMEOUT = 3.0
    TRIGGER1 = "Trigger1"
    TRIGGER2 = "Trigger2"
    TRIGGER3 = "Trigger3"

    # sphere state under test
    # fmt: off
    low_damping = SphereInfo(
        INITIAL_LINEAR_VELOCITY,
        5.0,
        Tests.enter_game_mode_low,
        Tests.find_sphere_low,
        Tests.exit_game_mode_low,
        Tests.find_triggers_low,
    )
    medium_damping = SphereInfo(
        INITIAL_LINEAR_VELOCITY,
        10.0,
        Tests.enter_game_mode_medium,
        Tests.find_sphere_medium,
        Tests.exit_game_mode_medium,
        Tests.find_triggers_medium,
    )
    high_damping = SphereInfo(
        INITIAL_LINEAR_VELOCITY,
        10000.0,
        Tests.enter_game_mode_high,
        Tests.find_sphere_high,
        Tests.exit_game_mode_high,
        Tests.find_triggers_high,
    )
    # fmt: on

    spheres = [low_damping, medium_damping, high_damping]

    helper.init_idle()
    # 1) Open level
    helper.open_level("Physics", "RigidBody_LinearDampingAffectsMotion")

    def run_test_steps(sphere):

        # 2) Enter game mode
        helper.enter_game_mode(sphere.enter_game_mode_test)

        # 3) Retrieve entities
        sphere.entity = general.find_game_entity("Sphere")
        Report.critical_result(sphere.find_sphere_test, sphere.entity.IsValid())

        triggers = [Trigger(TRIGGER1), Trigger(TRIGGER2), Trigger(TRIGGER3)]
        for trigger in triggers:
            trigger.entity = general.find_game_entity(trigger.name)

        invalid_triggers = [t for t in triggers if not t.entity.IsValid()]
        for trigger in invalid_triggers:
            Report.info("Trigger {} is invalid".format(trigger.name))
        Report.critical_result(sphere.triggers_test, len(invalid_triggers) == 0)

        # 4) Set sphere attributes, measure initial position, and then ForceAwake
        azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "SetLinearVelocity", sphere.entity, sphere.initial_linear_velocity)
        azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "SetLinearDamping", sphere.entity, sphere.linear_damping)
        sphere.initial_position = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTM", sphere.entity).GetPosition()

        # add handler for each trigger
        for trigger in triggers:
            trigger.handler = azlmbr.physics.TriggerNotificationBusHandler()
            trigger.handler.connect(trigger.entity)
            trigger.handler.add_callback("OnTriggerEnter", trigger.on_trigger)

        azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "ForceAwake", sphere.entity)
        general.idle_wait_frames(1)  # wait one frame for changes to apply

        # 5) Measure

        def sphere_stopped():
            sphere_linear_velocity = azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "GetLinearVelocity", sphere.entity)
            if sphere_linear_velocity.IsClose(ZERO_VELOCITY, TOLERANCE):
                sphere.final_position = azlmbr.components.TransformBus(
                    azlmbr.bus.Event, "GetWorldTM", sphere.entity
                ).GetPosition()
                for trigger in [t for t in triggers if t.triggered]:
                    if trigger.triggering_entity.Equal(sphere.entity):
                        sphere.triggers.append(trigger)
                return True
            return False

        if not helper.wait_for_condition(sphere_stopped, TIMEOUT):
            sphere.timeout = True
        sphere.report()

        # 6) Exit game mode
        helper.exit_game_mode(sphere.exit_game_mode_test)

    # 7) Run and repeat steps 2-6
    for sphere in spheres:
        run_test_steps(sphere)

    # 8) Report results
    no_timeout = True
    for sphere in spheres:
        if sphere.timeout:
            Report.info(
                "Timeout occurred. Sphere with damping = {} and Initial velocity = {} did not come to rest in the timeout of {}".format(
                    sphere.linear_damping, sphere.initial_linear_velocity, TIMEOUT
                )
            )
            no_timeout = False

    # fast fail if timeout occurred, comparisons will be meaningless and all info is in log to determine which sphere(s) timed out
    Report.critical_result(Tests.timeout, no_timeout)
    Report.result(
        Tests.high_damping_no_movement, high_damping.final_position.IsClose(high_damping.initial_position, TOLERANCE)
    )
    # comparative movement length
    Report.result(
        Tests.x_movement, high_damping.final_position.x < medium_damping.final_position.x < low_damping.final_position.x
    )
    # quantitative movement length
    all_three_tripped = (
        len(low_damping.triggers) == 3
        and len([t for t in low_damping.triggers if t.name == TRIGGER1]) == 1
        and len([t for t in low_damping.triggers if t.name == TRIGGER2]) == 1
        and len([t for t in low_damping.triggers if t.name == TRIGGER3]) == 1
    )
    Report.result(Tests.triggers_tripped_low, all_three_tripped)
    first_two_triggers_tripped = (
        len(medium_damping.triggers) == 2
        and len([t for t in medium_damping.triggers if t.name == TRIGGER1]) == 1
        and len([t for t in medium_damping.triggers if t.name == TRIGGER2]) == 1
    )
    Report.result(Tests.triggers_tripped_medium, first_two_triggers_tripped)
    Report.result(Tests.triggers_tripped_high, len(high_damping.triggers) == 0)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(RigidBody_LinearDampingAffectsMotion)
